#include "core/P4GCrypt.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ps4Atlus - Persona 4 Golden Test      \n";
    std::cout << "========================================\n\n";

    // 1. Verify PC .binslot dual-MD5 and P4GOLDEN signature
    std::cout << "[TEST 1] PC .binslot Signature and Salt Verification:\n";
    std::string pcSlot1 = "Save/Persona 4 golden/pc/data0001.binslot";
    std::string pcBin1 = "Save/Persona 4 golden/pc/data0001.bin";

    if (fs::exists(pcSlot1) && fs::exists(pcBin1)) {
        std::ifstream f(pcSlot1, std::ios::binary);
        std::vector<uint8_t> slotData((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        std::ifstream fb(pcBin1, std::ios::binary);
        std::vector<uint8_t> binData((std::istreambuf_iterator<char>(fb)), std::istreambuf_iterator<char>());
        fb.close();

        // 0x18 MD5
        uint8_t compMd5[16] = {0};
        Ps4Atlus::P4GCrypt::ComputeMD5(binData.data(), binData.size(), compMd5);
        assert(std::memcmp(slotData.data() + 0x18, compMd5, 16) == 0);
        std::cout << "  -> File MD5 at 0x18 matches byte-for-byte! [OK]\n";

        // 0x08 P4GOLDEN signature
        const char salt[] = "P4GOLDEN";
        std::vector<uint8_t> signPayload(slotData.begin() + 0x28, slotData.end());
        signPayload.insert(signPayload.end(), reinterpret_cast<const uint8_t*>(salt), reinterpret_cast<const uint8_t*>(salt) + 8);
        uint8_t compSig[16] = {0};
        Ps4Atlus::P4GCrypt::ComputeMD5(signPayload.data(), signPayload.size(), compSig);
        assert(std::memcmp(slotData.data() + 0x08, compSig, 16) == 0);
        std::cout << "  -> Signature with salt 'P4GOLDEN' at 0x08 matches byte-for-byte! [OK]\n";
    }

    // 2. Inspect PS4 Saves
    std::cout << "\n[TEST 2] Inspecting PS4 Saves:\n";
    struct ExpectedSave {
        std::string path;
        int expectedSlot;
        bool expectedSystem;
        bool expectedClear;
        int expectedLevel;
        std::string expectedName;
    };

    std::vector<ExpectedSave> ps4Items = {
        { "Save/Persona 4 golden/ps4/dec_SAVELIST0001_CUSA33874/GAME.BIN", 1, false, false, 89, "Damien Garreau" },
        { "Save/Persona 4 golden/ps4/dec_SAVELIST0002_CUSA33874/GAME.BIN", 2, false, false, 54, "Damien Garreau" },
        { "Save/Persona 4 golden/ps4/dec_SAVELIST0003_CUSA33874/GAME.BIN", 3, false, false, 83, "Damien Garreau" },
        { "Save/Persona 4 golden/ps4/dec_SAVELIST0004_CUSA33874/GAME.BIN", 4, false, false, 89, "Damien Garreau" },
        { "Save/Persona 4 golden/ps4/dec_SAVELIST0005_CUSA33874/GAME.BIN", 5, false, false, 1,  "Damien Garreau" },
        { "Save/Persona 4 golden/ps4/dec_SAVELIST0016_CUSA33874/GAME.BIN", 16, false, true, 1,  "kokiwa" },
        { "Save/Persona 4 golden/ps4/dec_CHANNELSAVE_CUSA33874/GAME.BIN", 0, true, false, 1,   "Données Système" }
    };

    for (const auto& item : ps4Items) {
        if (!fs::exists(item.path)) continue;

        int slot = 0;
        bool isSys = false;
        std::string dispName;
        Ps4Atlus::P4GCrypt::P4GMetadata meta;

        bool ok = Ps4Atlus::P4GCrypt::InspectSaveFile(item.path, slot, isSys, dispName, &meta);
        assert(ok);
        assert(slot == item.expectedSlot);
        assert(isSys == item.expectedSystem);
        assert(meta.isClear == item.expectedClear);
        if (!isSys) {
            assert(meta.level == item.expectedLevel);
            assert(meta.protagonistName == item.expectedName);
        }

        std::cout << "  " << dispName << ": " << meta.protagonistName
                  << " | Niv. " << meta.level
                  << " | Date: " << meta.inGameDate
                  << " | Temps: " << meta.playtimeFormatted
                  << (meta.isClear ? " [Terminé]" : "")
                  << " [OK]\n";
    }

    // 3. Test Conversion Pipeline
    std::cout << "\n[TEST 3] Converting PS4 Saves -> PC Steam Format:\n";
    fs::path testOutDir = "tests/test_output_p4g";
    fs::create_directories(testOutDir);

    for (const auto& item : ps4Items) {
        if (!fs::exists(item.path)) continue;

        std::string targetName = Ps4Atlus::P4GCrypt::FormatPcSaveName(item.expectedSlot, item.expectedSystem);
        fs::path outBin = testOutDir / targetName;
        fs::path outSlot = testOutDir / (targetName + "slot");

        std::string err;
        bool ok = Ps4Atlus::P4GCrypt::ConvertSaveFile(item.path, outBin.string(), outSlot.string(), item.expectedSlot, err);
        assert(ok);
        assert(fs::exists(outBin));
        assert(fs::file_size(outBin) == fs::file_size(item.path));
        assert(fs::exists(outSlot));
        assert(fs::file_size(outSlot) == Ps4Atlus::P4GCrypt::BinSlotSize);

        // Verify generated .binslot
        std::ifstream f(outSlot.string(), std::ios::binary);
        std::vector<uint8_t> genSlot((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        // 1. Verify bin MD5 at 0x18
        std::ifstream fb(outBin.string(), std::ios::binary);
        std::vector<uint8_t> genBin((std::istreambuf_iterator<char>(fb)), std::istreambuf_iterator<char>());
        fb.close();

        uint8_t compMd5[16] = {0};
        Ps4Atlus::P4GCrypt::ComputeMD5(genBin.data(), genBin.size(), compMd5);
        assert(std::memcmp(genSlot.data() + 0x18, compMd5, 16) == 0);

        // 2. Verify signature at 0x08
        const char salt[] = "P4GOLDEN";
        std::vector<uint8_t> signPayload(genSlot.begin() + 0x28, genSlot.end());
        signPayload.insert(signPayload.end(), reinterpret_cast<const uint8_t*>(salt), reinterpret_cast<const uint8_t*>(salt) + 8);
        uint8_t compSig[16] = {0};
        Ps4Atlus::P4GCrypt::ComputeMD5(signPayload.data(), signPayload.size(), compSig);
        assert(std::memcmp(genSlot.data() + 0x08, compSig, 16) == 0);

        // 3. Verify exact offsets and null padding
        for (size_t p = 0x28; p < 0x2c; ++p) {
            assert(genSlot[p] == 0);
        }
        for (size_t p = 0x68; p < 0x6c; ++p) {
            assert(genSlot[p] == 0);
        }
        for (size_t p = 0xe8; p < 0xec; ++p) {
            assert(genSlot[p] == 0);
        }
        for (size_t p = 0x2e0; p < 0x2ec; ++p) {
            assert(genSlot[p] == 0);
        }
        const char iconExpected[] = "app0:/data/icon.png";
        assert(std::memcmp(genSlot.data() + 0x2ec, iconExpected, sizeof(iconExpected) - 1) == 0);

        // Verify title at 0x2c
        assert(genSlot[0x2c] != 0);

        std::cout << "  " << targetName << " (" << genBin.size() << " bytes) + "
                  << targetName << "slot (" << genSlot.size() << " bytes) -> [CONVERSION VALID]\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "  TOUS LES TESTS P4G ONT REUSSI !       \n";
    std::cout << "========================================\n";
    return 0;
}
