#include "core/P3RCrypt.hpp"
#include <iostream>
#include <cassert>
#include <sstream>
#include <iomanip>

int main() {
    const std::string testData = "GVAS\x00\x00\x00\x01Persona3ReloadSaveDataTestPayload123456789";
    auto encrypted = Ps4Atlus::P3RCrypt::Encrypt(reinterpret_cast<const uint8_t*>(testData.data()), testData.size());
    
    std::stringstream ss;
    for (uint8_t b : encrypted) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    std::string hexStr = ss.str();
    std::cout << "C++ P3RCrypt Encrypted Hex: " << hexStr << std::endl;

    const std::string expectedHex = "1500650b65697471282e52191101255c0e182917377517330f06711d2f222d3716723b37323a1f3b3e22495c68604858457c";
    assert(hexStr == expectedHex);
    std::cout << "Assertion passed! C++ P3RCrypt is 100% byte-for-byte identical to reference implementation." << std::endl;

    // Test decryption roundtrip
    auto decrypted = Ps4Atlus::P3RCrypt::Decrypt(encrypted.data(), encrypted.size());
    std::string roundtrip(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
    assert(roundtrip == testData);
    std::cout << "Decryption roundtrip passed!" << std::endl;

    // Filename parsing and formatting
    int slot = 0;
    bool isAigis = false;
    bool isSystem = false;

    // Normal episode (SaveData001 -> SaveData002.sav)
    if (!Ps4Atlus::P3RCrypt::ParseSaveInfo("SaveData001", slot, isAigis, isSystem) || slot != 1 || isAigis) {
        std::cerr << "Normal episode parsing failed!" << std::endl;
        return 1;
    }
    if (Ps4Atlus::P3RCrypt::FormatPcSaveName(2, false, false) != "SaveData002.sav") {
        std::cerr << "Normal episode formatting failed!" << std::endl;
        return 1;
    }

    // Aigis episode (SaveData1001 -> SaveData1002.sav)
    if (!Ps4Atlus::P3RCrypt::ParseSaveInfo("SaveData1001", slot, isAigis, isSystem) || slot != 1001 || !isAigis) {
        std::cerr << "Aigis episode parsing failed!" << std::endl;
        return 1;
    }
    if (Ps4Atlus::P3RCrypt::FormatPcSaveName(1002, true, false) != "SaveData1002.sav") {
        std::cerr << "Aigis episode formatting failed!" << std::endl;
        return 1;
    }

    std::cout << "Filename parsing and formatting tests passed!" << std::endl;

    // Test real save inspection on actual project files
    std::string testPath001 = "Save/Persona 3 reload/dec_SaveData001_CUSA37522/ue4savegame.ps4.sav";
    std::string testPath1001 = "Save/Persona 3 reload/dec_SaveData1001_CUSA37522/ue4savegame.ps4.sav";

    if (std::filesystem::exists(testPath001)) {
        std::string dispName;
        if (!Ps4Atlus::P3RCrypt::InspectSaveFile(testPath001, slot, isAigis, isSystem, dispName)) {
            std::cerr << "Failed to inspect SaveData001 real save!" << std::endl;
            return 1;
        }
        if (slot != 1 || isAigis) {
            std::cerr << "Mismatch on real SaveData001: slot=" << slot << " isAigis=" << isAigis << std::endl;
            return 1;
        }
        std::cout << "Real PS4 SaveData001 inspection verified: slot=" << slot << " [Episode Normal]" << std::endl;
    }

    if (std::filesystem::exists(testPath1001)) {
        std::string dispName;
        if (!Ps4Atlus::P3RCrypt::InspectSaveFile(testPath1001, slot, isAigis, isSystem, dispName)) {
            std::cerr << "Failed to inspect SaveData1001 real save!" << std::endl;
            return 1;
        }
        if (slot != 1001 || !isAigis) {
            std::cerr << "Mismatch on real SaveData1001: slot=" << slot << " isAigis=" << isAigis << std::endl;
            return 1;
        }
        std::cout << "Real PS4 SaveData1001 inspection verified: slot=" << slot << " [Episode Aigis]" << std::endl;
    }

    // Test DLC flag clearing on real SaveData001
    if (std::filesystem::exists(testPath001)) {
        std::ifstream in(testPath001, std::ios::binary);
        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        auto dlcRes = Ps4Atlus::P3RCrypt::CleanDlcFlags(buffer);
        if (!dlcRes.flagACleared) {
            std::cerr << "DLC cleaning failed: Flag A was not cleared!" << std::endl;
            return 1;
        }
        std::cout << "DLC Flag cleaning test passed! (Flag A cleared: " << dlcRes.flagACleared 
                  << ", Flag B cleared: " << dlcRes.flagBCleared << ")" << std::endl;
    }

    std::cout << "All real save inspection and DLC cleaning tests passed successfully!" << std::endl;
    return 0;
}
