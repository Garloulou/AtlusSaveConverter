#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <filesystem>
#include "core/P5SCrypt.hpp"

namespace fs = std::filesystem;

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ps4Atlus - Persona 5 Strikers Test    \n";
    std::cout << "========================================\n\n";

    // 1. Test Symmetry of LCG XOR Crypt
    std::cout << "[TEST 1] Crypt Function Symmetry:\n";
    std::vector<uint8_t> testData(1024, 0x55);
    std::vector<uint8_t> original = testData;
    uint64_t testSteamId = 1236673082ULL;

    Ps4Atlus::P5SCrypt::Crypt(testData.data(), testData.size(), testSteamId);
    assert(testData != original);
    assert(testData[testData.size() - 4] != 0); // Checksum written

    Ps4Atlus::P5SCrypt::Crypt(testData.data(), testData.size(), testSteamId);
    // Data before checksum must match
    assert(std::memcmp(testData.data(), original.data(), original.size() - 4) == 0);
    std::cout << "  -> Crypt/Decrypt roundtrip successful [OK]\n\n";

    // 2. Test Decryption and Inspection of Real User P5S Save
    std::string userSavePath = "C:\\Users\\garre\\AppData\\Roaming\\SEGA\\Steam\\P5S\\1028152820\\SAVEDATA.BIN";
    if (fs::exists(userSavePath)) {
        std::cout << "[TEST 2] Decrypting Real User Save (1028152820):\n";
        Ps4Atlus::P5SMetadata meta;
        std::string displayName;
        bool ok = Ps4Atlus::P5SCrypt::InspectSaveFile(userSavePath, 1028152820ULL, meta, displayName);
        assert(ok);
        assert(!meta.isEncrypted || meta.activeSlot != -1);
        std::cout << "  Active slot: " << meta.activeSlot << "\n";
        std::cout << "  Detected format: " << (meta.detectedFormat == Ps4Atlus::P5SSaveFormat::PC ? "PC" : "Other") << "\n";
        assert(meta.detectedFormat == Ps4Atlus::P5SSaveFormat::PC);

        bool foundDamien = false;
        bool foundIgorie = false;
        for (const auto& s : meta.slots) {
            if (!s.fullName.empty()) {
                std::cout << "  Slot " << (s.slotIndex + 1) << ": " << s.fullName 
                          << " | Niv. " << s.level << " | Argent: " << s.money << "\n";
                if (s.fullName.find("Damien") != std::string::npos) foundDamien = true;
                if (s.fullName.find("Igorie") != std::string::npos) foundIgorie = true;
            }
        }
        assert(foundDamien);
        assert(foundIgorie);
        std::cout << "  -> Real user save inspected and verified [OK]\n\n";
    }

    // 3. Test PS4_EN -> PC Conversion with synthetic data
    std::cout << "[TEST 3] Simulated PS4_EN -> PC Conversion:\n";
    size_t ps4Size = Ps4Atlus::P5SCrypt::FORMAT_PS4_EN.size;
    std::vector<uint8_t> ps4Buffer(ps4Size, 0);

    // Set header version and active slot
    *reinterpret_cast<uint32_t*>(ps4Buffer.data() + 0) = 0x20012000;
    *reinterpret_cast<int32_t*>(ps4Buffer.data() + 4) = 1; // Slot 2 active

    // Set platform magic at slot 1 end + 0x938
    size_t magicOff = 0x1C + 0x88DF4 + 33 * 2 + 0x938;
    *reinterpret_cast<uint32_t*>(ps4Buffer.data() + magicOff) = 0x0036EE7F;

    // Set protagonist names in slot 2 (index 1)
    size_t slot1Start = 0x1C + 1 * (0x88DF4 + 33 * 2);
    size_t nameOff = slot1Start + 0x87842;
    std::string testFname = "Joker";
    std::string testLname = "Kurusu";
    std::memcpy(ps4Buffer.data() + nameOff, testFname.c_str(), testFname.size());
    std::memcpy(ps4Buffer.data() + nameOff + 33, testLname.c_str(), testLname.size());

    // Verify auto-detection of PS4_EN format
    auto detectedFmt = Ps4Atlus::P5SCrypt::DetectFormat(ps4Buffer.data(), ps4Buffer.size());
    assert(detectedFmt.format == Ps4Atlus::P5SSaveFormat::PS4_EN);
    std::cout << "  Detected input format: PS4_EN [OK]\n";

    // Convert to PC
    std::vector<uint8_t> pcBuffer;
    bool convOk = Ps4Atlus::P5SCrypt::Convert(ps4Buffer.data(), ps4Buffer.size(),
                                             Ps4Atlus::P5SCrypt::FORMAT_PS4_EN,
                                             Ps4Atlus::P5SCrypt::FORMAT_PC,
                                             pcBuffer);
    assert(convOk);
    assert(pcBuffer.size() == Ps4Atlus::P5SCrypt::FORMAT_PC.size);
    std::cout << "  Output buffer size: " << pcBuffer.size() << " bytes [OK]\n";

    // Verify names survived the conversion
    size_t pcSlot1Start = 0x1C + 1 * (0x88DF4 + 33 * 2);
    size_t pcNameOff = pcSlot1Start + 0x87842;
    std::string outFname(reinterpret_cast<const char*>(pcBuffer.data() + pcNameOff));
    std::string outLname(reinterpret_cast<const char*>(pcBuffer.data() + pcNameOff + 33));
    assert(outFname == "Joker");
    assert(outLname == "Kurusu");
    std::cout << "  Protagonist names verified: " << outFname << " " << outLname << " [OK]\n\n";

    // 4. Test ConvertSaveFile with encryption
    std::cout << "[TEST 4] ConvertSaveFile to Output File:\n";
    std::string testInputPath = "Save/test_p5s_ps4.bin";
    std::string testOutputPath = "Save/test_p5s_pc/SAVEDATA.BIN";
    {
        std::ofstream out(testInputPath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(ps4Buffer.data()), ps4Buffer.size());
    }

    std::string err;
    bool fullOk = Ps4Atlus::P5SCrypt::ConvertSaveFile(testInputPath, testOutputPath, 1236673082ULL, err);
    assert(fullOk);
    assert(fs::exists(testOutputPath));
    assert(fs::file_size(testOutputPath) == Ps4Atlus::P5SCrypt::FORMAT_PC.size);

    // Verify it is encrypted
    {
        std::ifstream in(testOutputPath, std::ios::binary);
        std::vector<uint8_t> testPc((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        assert(Ps4Atlus::P5SCrypt::IsEncrypted(testPc.data(), testPc.size()));

        // Decrypt with target steamId
        Ps4Atlus::P5SCrypt::Crypt(testPc.data(), testPc.size(), 1236673082ULL);
        assert(!Ps4Atlus::P5SCrypt::IsEncrypted(testPc.data(), testPc.size()));
        std::cout << "  -> File encrypted and verified decryptable with target SteamID [OK]\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "  TOUS LES TESTS P5S ONT REUSSI !       \n";
    std::cout << "========================================\n";
    return 0;
}
