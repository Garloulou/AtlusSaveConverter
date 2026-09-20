#include "core/P3PCrypt.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ps4Atlus - Persona 3 Portable Test    \n";
    std::cout << "========================================\n\n";

    // 1. Test MD5 on known PC save P3PSAVE0001.BIN
    std::string pcBinPath = "Save/Persona 3 portable/PC/P3PSAVE0001.BIN";
    std::string pcSlotPath = "Save/Persona 3 portable/PC/P3PSAVE0001.BINslot";

    if (fs::exists(pcBinPath) && fs::exists(pcSlotPath)) {
        std::ifstream binF(pcBinPath, std::ios::binary);
        std::vector<uint8_t> binData((std::istreambuf_iterator<char>(binF)), std::istreambuf_iterator<char>());
        binF.close();

        uint8_t digest[16] = {0};
        [[maybe_unused]] bool md5Ok = Ps4Atlus::P3PCrypt::CalculateMd5(binData.data(), binData.size(), digest);
        assert(md5Ok);

        std::string hexMd5 = Ps4Atlus::P3PCrypt::DigestToHex(digest);
        std::cout << "[TEST 1] Known PC Save MD5: " << hexMd5 << "\n";
        assert(hexMd5 == "e4d8f1c2cc66fb4bb6088b25eff71188");

        std::ifstream slotF(pcSlotPath, std::ios::binary);
        std::vector<uint8_t> slotData((std::istreambuf_iterator<char>(slotF)), std::istreambuf_iterator<char>());
        slotF.close();

        assert(slotData.size() == 1340);
        assert(std::memcmp(&slotData[0x18], digest, 16) == 0);
        std::cout << "  -> MD5 matches byte-for-byte in P3PSAVE0001.BINslot at offset 0x18!\n";
    }

    // 2. Test metadata inspection across PS4 saves
    std::cout << "\n[TEST 2] Inspecting PS4 Saves:\n";
    struct ExpectedSave {
        std::string path;
        int slot;
        std::string expectedName;
        int expectedLevel;
        int expectedGender; // 0=Male, 1=Female
        std::string expectedDate;
        std::string expectedTime;
        bool expectedClear;
    };

    std::vector<ExpectedSave> ps4Saves = {
        { "Save/Persona 3 portable/PS4/dec_SAVELIST0001_CUSA33872/GAME.BIN", 1, "Eupha Shiomi", 93, 1, "03/04", "99h 10m", false },
        { "Save/Persona 3 portable/PS4/dec_SAVELIST0002_CUSA33872/GAME.BIN", 2, "Eupha Shiomi", 93, 1, "04/06", "99h 19m", true },
        { "Save/Persona 3 portable/PS4/dec_SAVELIST0003_CUSA33872/GAME.BIN", 3, "Damien Garreau", 93, 0, "04/20", "99h 29m", false },
        { "Save/Persona 3 portable/PS4/dec_SAVELIST0004_CUSA33872/GAME.BIN", 4, "Damien Garreau", 99, 0, "04/24", "103h 08m", false },
        { "Save/Persona 3 portable/PS4/dec_SAVELIST0014_CUSA33872/GAME.BIN", 14, "Eupha Shiomi", 47, 1, "10/06", "69h 15m", false },
    };

    for (const auto& item : ps4Saves) {
        if (!fs::exists(item.path)) {
            std::cout << "  Skipping missing: " << item.path << "\n";
            continue;
        }

        int slot = 0;
        bool isSystem = false;
        std::string displayName;
        Ps4Atlus::P3PCrypt::P3PMetadata meta;
        [[maybe_unused]] bool ok = Ps4Atlus::P3PCrypt::InspectSaveFile(item.path, slot, isSystem, displayName, &meta);
        assert(ok);
        assert(!isSystem);
        assert(slot == item.slot);
        assert(meta.protagonistName == item.expectedName);
        assert(meta.level == item.expectedLevel);
        assert(meta.gender == item.expectedGender);
        assert(meta.inGameDate == item.expectedDate);
        assert(meta.playtimeFormatted == item.expectedTime);
        assert(meta.isClear == item.expectedClear);

        std::cout << "  Slot " << slot << " (" << displayName << "): "
                  << meta.protagonistName << " | Niv. " << meta.level
                  << " | " << meta.genderStr
                  << " | Date: " << meta.inGameDate
                  << " | HP: " << meta.maxHp << ", SP: " << meta.maxSp
                  << " | Temps: " << meta.playtimeFormatted
                  << (meta.isClear ? " [Terminé]" : "") << " [OK]\n";
    }

    // 3. Test system save inspection
    std::string sysPath = "Save/Persona 3 portable/PS4/dec_SYSTEMSAVE_CUSA33872/GAME.BIN";
    if (fs::exists(sysPath)) {
        int slot = 0;
        bool isSystem = false;
        std::string displayName;
        Ps4Atlus::P3PCrypt::P3PMetadata meta;
        [[maybe_unused]] bool ok = Ps4Atlus::P3PCrypt::InspectSaveFile(sysPath, slot, isSystem, displayName, &meta);
        assert(ok);
        assert(isSystem);
        assert(slot == 0);
        assert(displayName == "SYSTEM.BIN");
        std::cout << "  System save: " << displayName << " [OK]\n";
    }

    // 4. Test complete conversion pipeline (BIN + BINslot)
    std::cout << "\n[TEST 3] Converting PS4 Saves -> PC Steam Format:\n";
    fs::path testOutDir = "tests/test_output_p3p";
    fs::create_directories(testOutDir);

    for (const auto& item : ps4Saves) {
        if (!fs::exists(item.path)) continue;

        char binName[64], slotName[64];
        std::snprintf(binName, sizeof(binName), "P3PSAVE%04d.BIN", item.slot);
        std::snprintf(slotName, sizeof(slotName), "P3PSAVE%04d.BINslot", item.slot);

        fs::path outBin = testOutDir / binName;
        fs::path outSlot = testOutDir / slotName;

        std::string err;
        bool convOk = Ps4Atlus::P3PCrypt::ConvertSaveFile(item.path, outBin.string(), outSlot.string(), item.slot, err);
        if (!convOk) {
            std::cerr << "  Conversion failed for " << binName << ": " << err << "\n";
            return 1;
        }

        // Verify .BIN size
        uintmax_t binSize = fs::file_size(outBin);
        assert(binSize == Ps4Atlus::P3PCrypt::PcGameSaveSizeBytes);

        // Verify .BINslot size
        uintmax_t slotSize = fs::file_size(outSlot);
        assert(slotSize == Ps4Atlus::P3PCrypt::BinSlotSizeBytes);

        // Verify .BINslot content & MD5 check
        std::ifstream outBinF(outBin, std::ios::binary);
        std::vector<uint8_t> outBinData((std::istreambuf_iterator<char>(outBinF)), std::istreambuf_iterator<char>());
        outBinF.close();

        uint8_t calcDigest[16];
        Ps4Atlus::P3PCrypt::CalculateMd5(outBinData.data(), outBinData.size(), calcDigest);

        std::ifstream outSlotF(outSlot, std::ios::binary);
        std::vector<uint8_t> outSlotData((std::istreambuf_iterator<char>(outSlotF)), std::istreambuf_iterator<char>());
        outSlotF.close();

        // Check slot identifier string
        char expectedSlotTag[16];
        std::snprintf(expectedSlotTag, sizeof(expectedSlotTag), "SAVE%04d", item.slot);
        assert(std::memcmp(&outSlotData[0], expectedSlotTag, 8) == 0);

        // Check MD5 at offset 0x18
        assert(std::memcmp(&outSlotData[0x18], calcDigest, 16) == 0);

        // Check header signature at offset 0x08: MD5(slotData[0x28:] + "P3POTABL")
        uint8_t expectedSig[16];
        Ps4Atlus::P3PCrypt::CalculateMd5WithSalt(&outSlotData[0x28], 1340 - 0x28, "P3POTABL", 8, expectedSig);
        assert(std::memcmp(&outSlotData[0x08], expectedSig, 16) == 0);

        // Check CLEAR DATA text block if clear save
        if (item.expectedClear) {
            assert(std::memcmp(&outSlotData[0x128], "CLEAR DATA\n", 11) == 0);
        } else {
            assert(std::memcmp(&outSlotData[0x128], "CLEAR DATA\n", 11) != 0);
        }

        std::cout << "  " << binName << " (" << binSize << " bytes) + "
                  << slotName << " (" << slotSize << " bytes, MD5: "
                  << Ps4Atlus::P3PCrypt::DigestToHex(calcDigest)
                  << ", Sig: " << Ps4Atlus::P3PCrypt::DigestToHex(expectedSig) << ") -> [CONVERSION VALID]\n";
    }

    // 5. Test system save conversion
    if (fs::exists(sysPath)) {
        fs::path outSys = testOutDir / "SYSTEM.BIN";
        std::string err;
        [[maybe_unused]] bool convSysOk = Ps4Atlus::P3PCrypt::ConvertSaveFile(sysPath, outSys.string(), "", 0, err);
        assert(convSysOk);
        assert(fs::file_size(outSys) == Ps4Atlus::P3PCrypt::SysSaveSizeBytes);
        std::cout << "  SYSTEM.BIN (" << fs::file_size(outSys) << " bytes) -> [CONVERSION VALID]\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "  TOUS LES TESTS P3P ONT REUSSI !       \n";
    std::cout << "========================================\n";
    return 0;
}
