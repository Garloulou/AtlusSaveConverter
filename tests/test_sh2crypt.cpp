#include "core/SH2Crypt.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ps4Atlus - Soul Hackers 2 Test        \n";
    std::cout << "========================================\n\n";

    std::string testSaveDataDir = "C:\\Users\\garre\\AppData\\Roaming\\SEGA\\SOULHACKERS2\\Steam\\1099287329\\SaveData";

    // 1. Test Steam ID detection
    std::cout << "[TEST 1] Steam ID Detection:\n";
    if (fs::exists(testSaveDataDir)) {
        std::string steamId = Ps4Atlus::SH2Crypt::DetectSteamId(testSaveDataDir);
        std::cout << "  Detected Steam ID: " << steamId << "\n";
        assert(steamId == "1099287329");
        std::cout << "  -> [OK]\n";
    } else {
        std::cout << "  Skipping test 1 (path not found)\n";
    }

    // 2. Test system.dat parsing
    std::cout << "\n[TEST 2] Parsing Real system.dat Files:\n";
    std::string s1Path = testSaveDataDir + "\\SAVE01\\system.dat";
    if (fs::exists(s1Path)) {
        Ps4Atlus::SH2Crypt::SH2Metadata meta;
        bool parsed = Ps4Atlus::SH2Crypt::ParseSystemDat(s1Path, meta);
        assert(parsed);
        assert(meta.slotNumber == 1);
        assert(!meta.isSystem);
        assert(!meta.isAutoSave);
        assert(meta.protagonistName == "Ringo");
        assert(meta.level == 38);
        assert(meta.partySize == 4);
        assert(meta.playtimeHours == 19);
        assert(meta.playtimeMinutes == 8);
        assert(meta.difficulty == "EASY");

        std::cout << "  SAVE01: " << meta.protagonistName << " | Niv. " << meta.level
                  << " | " << meta.difficulty << " | Temps: " << meta.playtimeFormatted
                  << " | Membres: " << meta.partySize << " [OK]\n";
    }

    std::string s2Path = testSaveDataDir + "\\SAVE02\\system.dat";
    if (fs::exists(s2Path)) {
        Ps4Atlus::SH2Crypt::SH2Metadata meta;
        bool parsed = Ps4Atlus::SH2Crypt::ParseSystemDat(s2Path, meta);
        assert(parsed);
        assert(meta.slotNumber == 2);
        assert(meta.level == 2);
        assert(meta.partySize == 2);
        assert(meta.playtimeHours == 0);
        assert(meta.playtimeMinutes == 34);
        assert(meta.difficulty == "NORMAL");

        std::cout << "  SAVE02: " << meta.protagonistName << " | Niv. " << meta.level
                  << " | " << meta.difficulty << " | Temps: " << meta.playtimeFormatted
                  << " | Membres: " << meta.partySize << " [OK]\n";
    }

    std::string autoPath = testSaveDataDir + "\\AUTOSAVE\\system.dat";
    if (fs::exists(autoPath)) {
        Ps4Atlus::SH2Crypt::SH2Metadata meta;
        bool parsed = Ps4Atlus::SH2Crypt::ParseSystemDat(autoPath, meta);
        assert(parsed);
        assert(meta.isAutoSave);
        assert(meta.level == 38);

        std::cout << "  AUTOSAVE: " << meta.protagonistName << " | Niv. " << meta.level
                  << " | " << meta.difficulty << " | Temps: " << meta.playtimeFormatted << " [OK]\n";
    }

    std::string sysPath = testSaveDataDir + "\\SYSTEMDATA\\system.dat";
    if (fs::exists(sysPath)) {
        Ps4Atlus::SH2Crypt::SH2Metadata meta;
        bool parsed = Ps4Atlus::SH2Crypt::ParseSystemDat(sysPath, meta);
        assert(parsed);
        assert(meta.isSystem);

        std::cout << "  SYSTEMDATA: isSystem = true [OK]\n";
    }

    // 3. Test Save File Inspection (data.dat)
    std::cout << "\n[TEST 3] Save File Inspection:\n";
    std::string data1Path = testSaveDataDir + "\\SAVE01\\data.dat";
    if (fs::exists(data1Path)) {
        int slot = 0;
        bool isSys = false;
        bool isAuto = false;
        std::string dispName;
        Ps4Atlus::SH2Crypt::SH2Metadata meta;
        bool ok = Ps4Atlus::SH2Crypt::InspectSaveFile(data1Path, slot, isSys, isAuto, dispName, &meta);
        assert(ok);
        assert(slot == 1);
        assert(!isSys);
        assert(!isAuto);
        assert(dispName == "SAVE01");
        assert(meta.level == 38);

        std::cout << "  " << data1Path << " -> " << dispName << " (Slot " << slot
                  << ", Niv. " << meta.level << ") [OK]\n";
    }

    // 4. Test Save Conversion to Target Directory
    std::cout << "\n[TEST 4] Save Conversion Pipeline:\n";
    fs::path testOutDir = "tests/test_output_sh2";
    fs::create_directories(testOutDir);

    if (fs::exists(data1Path)) {
        std::string err;
        bool convOk = Ps4Atlus::SH2Crypt::ConvertSaveFile(
            data1Path, testOutDir.string(), 1, false, false, false, "1099287329", err
        );
        assert(convOk);

        fs::path outDataDat = testOutDir / "SAVE01" / "data.dat";
        fs::path outSysDat = testOutDir / "SAVE01" / "system.dat";
        fs::path outVdf = testOutDir / "SAVE01" / "steam_autocloud.vdf";

        assert(fs::exists(outDataDat));
        assert(fs::file_size(outDataDat) == fs::file_size(data1Path));
        assert(fs::exists(outSysDat));
        assert(fs::file_size(outSysDat) > 0);
        assert(fs::exists(outVdf));
        assert(fs::file_size(outVdf) > 0);

        // Verify generated system.dat can be parsed back
        Ps4Atlus::SH2Crypt::SH2Metadata checkMeta;
        bool checkParsed = Ps4Atlus::SH2Crypt::ParseSystemDat(outSysDat.string(), checkMeta);
        assert(checkParsed);
        assert(checkMeta.slotNumber == 1);
        assert(checkMeta.level == 38);

        std::cout << "  SAVE01 Conversion -> " << outDataDat.string() << " ("
                  << fs::file_size(outDataDat) << " bytes) + system.dat + steam_autocloud.vdf [OK]\n";
    }

    // 5. Test DLC Requirement Cleaning in system.dat
    std::cout << "\n[TEST 5] DLC Flag Cleaning in system.dat:\n";
    std::string testDlcSys = "Save/Soul Hacker 2/dec_SAVE01_CUSA27730/system.dat";
    if (fs::exists(testDlcSys)) {
        std::ifstream f(testDlcSys, std::ios::binary);
        std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        bool cleaned = false;
        bool res = Ps4Atlus::SH2Crypt::CleanDlcFlagsInSystemDat(raw, cleaned);
        assert(res);
        assert(cleaned);

        // Verify it was changed to 2147483648
        std::string txt;
        for (size_t i = 0; i + 1 < raw.size(); i += 2) {
            uint16_t ch = raw[i] | (raw[i + 1] << 8);
            if (ch < 128) txt.push_back(static_cast<char>(ch));
        }
        assert(txt.find("\"userParam\": 2147483648") != std::string::npos);

        // Run again, should not clean again
        bool cleanedAgain = false;
        res = Ps4Atlus::SH2Crypt::CleanDlcFlagsInSystemDat(raw, cleanedAgain);
        assert(res);
        assert(!cleanedAgain);

        std::cout << "  userParam 2147484673 -> 2147483648 (DLC stripped successfully) [OK]\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "  TOUS LES TESTS SH2 ONT REUSSI !       \n";
    std::cout << "========================================\n";
    return 0;
}
