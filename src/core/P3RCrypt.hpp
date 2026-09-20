#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <filesystem>
#include <cstring>
#include <algorithm>

namespace Ps4Atlus {

class P3RCrypt {
public:
    // P3RCrypt encryption key from https://github.com/dev-camo/P3RCrypt
    static constexpr std::string_view Key = "ae5zeitaix1joowooNgie3fahP5Ohph";

    // Encrypt a single byte using P3RCrypt bit transformation and XOR
    static inline uint8_t EncryptByte(uint8_t data, uint8_t key) {
        return ((((data & 0xff) >> 4) & 3) | ((data & 3) << 4) | (data & 0xcc)) ^ key;
    }

    // Decrypt a single byte using inverse bit transformation and XOR
    static inline uint8_t DecryptByte(uint8_t data, uint8_t key) {
        uint8_t bVar1 = data ^ key;
        return (((bVar1 >> 4) & 3) | ((bVar1 & 3) << 4) | (bVar1 & 0xcc));
    }

    // Encrypt in-memory buffer
    static std::vector<uint8_t> Encrypt(const uint8_t* data, size_t size) {
        std::vector<uint8_t> result(size);
        const size_t keyLen = Key.size();
        for (size_t i = 0; i < size; ++i) {
            uint8_t k = static_cast<uint8_t>(Key[i % keyLen]);
            result[i] = EncryptByte(data[i], k);
        }
        return result;
    }

    // Decrypt in-memory buffer
    static std::vector<uint8_t> Decrypt(const uint8_t* data, size_t size) {
        std::vector<uint8_t> result(size);
        const size_t keyLen = Key.size();
        for (size_t i = 0; i < size; ++i) {
            uint8_t k = static_cast<uint8_t>(Key[i % keyLen]);
            result[i] = DecryptByte(data[i], k);
        }
        return result;
    }

    struct DlcCleanResult {
        bool flagACleared = false;
        bool flagBCleared = false;
        bool flagCCleared = false;

        bool AnyCleared() const {
            return flagACleared || flagBCleared || flagCCleared;
        }
    };

    // Clean DLC flags that prevent saves from loading on PC
    // Scans the unencrypted GVAS buffer for DLC requirement bitfields (A, B, C) and clears them
    static DlcCleanResult CleanDlcFlags(std::vector<uint8_t>& buf) {
        DlcCleanResult result;
        if (buf.size() < 1024) return result;

        const size_t searchLimit = std::min<size_t>(buf.size(), 0x10000); // scan first 64 KB

        // Flag Set A: SaveDataArea ... UInt32Property ... 0x98 0x01 0x00 0x00 0x00
        // Controls DLC bitfield at index 408 (includes the PS4 DLC flag 0x20)
        static const uint8_t dlcflagsetA[] = {
            0x53, 0x61, 0x76, 0x65, 0x44, 0x61, 0x74, 0x61,
            0x41, 0x72, 0x65, 0x61, 0x00, 0x0F, 0x00, 0x00,
            0x00, 0x55, 0x49, 0x6E, 0x74, 0x33, 0x32, 0x50,
            0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x98, 0x01, 0x00, 0x00,
            0x00
        };
        const size_t lenA = sizeof(dlcflagsetA);

        for (size_t i = 0; i + lenA + 2 <= searchLimit; ++i) {
            if (std::memcmp(buf.data() + i, dlcflagsetA, lenA) == 0) {
                size_t pos = i + lenA;
                if (buf[pos] != 0x00 || buf[pos + 1] != 0x00) {
                    buf[pos] = 0x00;
                    buf[pos + 1] = 0x00;
                    result.flagACleared = true;
                }
                break;
            }
        }

        // Flag Set B: SaveDataArea ... UInt32Property ... 0x9B
        static const uint8_t dlcflagsetB[] = {
            0x53, 0x61, 0x76, 0x65, 0x44, 0x61, 0x74, 0x61,
            0x41, 0x72, 0x65, 0x61, 0x00, 0x0F, 0x00, 0x00,
            0x00, 0x55, 0x49, 0x6E, 0x74, 0x33, 0x32, 0x50,
            0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x9B
        };
        const size_t lenB = sizeof(dlcflagsetB);

        for (size_t i = 0; i + lenB <= searchLimit; ++i) {
            if (std::memcmp(buf.data() + i, dlcflagsetB, lenB) == 0) {
                size_t pos = i + lenB;
                if (buf[pos - 1] == 0x9B) {
                    buf[pos - 1] = 0x97;
                    result.flagBCleared = true;
                }
                break;
            }
        }

        // Flag Set C: SaveDataArea ... UInt32Property ... 0x97 0x01 0x00 0x00 0x00
        static const uint8_t dlcflagsetC[] = {
            0x53, 0x61, 0x76, 0x65, 0x44, 0x61, 0x74, 0x61,
            0x41, 0x72, 0x65, 0x61, 0x00, 0x0F, 0x00, 0x00,
            0x00, 0x55, 0x49, 0x6E, 0x74, 0x33, 0x32, 0x50,
            0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x97, 0x01, 0x00, 0x00,
            0x00
        };
        const size_t lenC = sizeof(dlcflagsetC);
        static const uint8_t dlcflagtail[] = { 0x80, 0x0D, 0x00, 0x00, 0x00 };
        const size_t lenTail = sizeof(dlcflagtail);

        for (size_t i = 0; i + lenC + 3 + lenTail <= searchLimit; ++i) {
            if (std::memcmp(buf.data() + i, dlcflagsetC, lenC) == 0) {
                size_t tailPos = i + lenC + 3;
                if (std::memcmp(buf.data() + tailPos, dlcflagtail, lenTail) == 0) {
                    buf[tailPos] = 0x00;
                    result.flagCCleared = true;
                }
                break;
            }
        }

        return result;
    }

    // Process file to file with optional DLC flags removal and P3RCrypt encryption
    static bool EncryptSaveFile(const std::string& inputPath, const std::string& outputPath, bool removeDlcFlags, DlcCleanResult& outDlcResult, std::string& outError) {
        std::ifstream inFile(inputPath, std::ios::binary);
        if (!inFile) {
            outError = "Impossible d'ouvrir le fichier source : " + inputPath;
            return false;
        }

        inFile.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        if (fileSize == 0) {
            outError = "Le fichier source est vide (0 octet).";
            return false;
        }

        std::vector<uint8_t> inBuffer(fileSize);
        if (!inFile.read(reinterpret_cast<char*>(inBuffer.data()), fileSize)) {
            outError = "Erreur de lecture du fichier source.";
            return false;
        }
        inFile.close();

        // Remove DLC requirements if requested
        if (removeDlcFlags) {
            outDlcResult = CleanDlcFlags(inBuffer);
        }

        // Encrypt with P3RCrypt
        std::vector<uint8_t> encrypted = Encrypt(inBuffer.data(), inBuffer.size());

        // Write output
        std::ofstream outFile(outputPath, std::ios::binary | std::ios::trunc);
        if (!outFile) {
            outError = "Impossible de créer le fichier de destination : " + outputPath;
            return false;
        }

        if (!outFile.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size())) {
            outError = "Erreur d'écriture dans le fichier de destination.";
            return false;
        }
        outFile.close();

        return true;
    }

    // Overload for simple call without DLC flag parameters
    static bool EncryptSaveFile(const std::string& inputPath, const std::string& outputPath, std::string& outError) {
        DlcCleanResult dummy;
        return EncryptSaveFile(inputPath, outputPath, true, dummy, outError);
    }

    // Helper to analyze slot and episode from filename
    // Supports formats like SaveData001, SaveData0001, SaveData1, SaveData1001, SaveData1002.sav etc.
    static bool ParseSaveInfo(const std::string& filename, int& outSlot, bool& outIsAigis, bool& outIsSystem) {
        outSlot = 1;
        outIsAigis = false;
        outIsSystem = false;

        std::string lowerName = filename;
        for (char& c : lowerName) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

        if (lowerName.find("system") != std::string::npos) {
            outIsSystem = true;
            outSlot = 0;
            return true;
        }

        // Search for digits in the filename
        std::regex re("(\\d+)");
        std::smatch match;
        if (std::regex_search(filename, match, re) && match.size() > 1) {
            try {
                int num = std::stoi(match[1].str());
                outSlot = num;
                if (outSlot >= 1000) {
                    outIsAigis = true;
                } else {
                    outIsAigis = false;
                }
                return true;
            } catch (...) {
                // Fallback
            }
        }

        return false;
    }

    // Inspects save file using BOTH binary content (GVAS properties) and folder/filename patterns
    static bool InspectSaveFile(const std::string& filePath, int& outSlot, bool& outIsAigis, bool& outIsSystem, std::string& outDisplayName) {
        namespace fs = std::filesystem;
        outSlot = 1;
        outIsAigis = false;
        outIsSystem = false;

        fs::path p(filePath);
        std::string fname = p.filename().string();
        std::string parentName = p.parent_path().filename().string();

        bool foundFromBinary = false;

        // 1. Binary inspection: read first 8192 bytes
        std::ifstream file(filePath, std::ios::binary);
        if (file) {
            std::vector<char> buffer(8192);
            file.read(buffer.data(), buffer.size());
            std::streamsize bytesRead = file.gcount();
            std::string_view data(buffer.data(), static_cast<size_t>(bytesRead));

            // Episode Aigis check in GVAS class (AstreaSaveGame is Episode Aigis)
            if (data.find("AstreaSaveGame") != std::string_view::npos) {
                outIsAigis = true;
            }

            // SaveSlotName check in GVAS properties
            size_t slotNamePos = data.find("SaveSlotName");
            if (slotNamePos != std::string_view::npos) {
                size_t sdPos = data.find("SaveData", slotNamePos);
                if (sdPos != std::string_view::npos && (sdPos - slotNamePos) < 120) {
                    std::string rawSlotStr;
                    for (size_t i = sdPos; i < data.size() && data[i] != '\0' && i < sdPos + 24; ++i) {
                        rawSlotStr.push_back(data[i]);
                    }
                    std::regex re("(\\d+)");
                    std::smatch m;
                    if (std::regex_search(rawSlotStr, m, re)) {
                        try {
                            outSlot = std::stoi(m[1].str());
                            if (outSlot >= 1000) outIsAigis = true;
                            foundFromBinary = true;
                        } catch (...) {}
                    }
                }
            }
        }

        // 2. Fallback / confirmation with parent folder name or filename (e.g. dec_SaveData001_CUSA37522 or dec_SaveData1001_CUSA37522)
        if (!foundFromBinary) {
            std::regex re("SaveData(\\d+)");
            std::smatch m;
            if (std::regex_search(parentName, m, re)) {
                try {
                    outSlot = std::stoi(m[1].str());
                    if (outSlot >= 1000) outIsAigis = true;
                } catch (...) {}
            } else if (std::regex_search(fname, m, re)) {
                try {
                    outSlot = std::stoi(m[1].str());
                    if (outSlot >= 1000) outIsAigis = true;
                } catch (...) {}
            }
        }

        // 3. System data check
        std::string lowerFull = filePath;
        for (char& c : lowerFull) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (lowerFull.find("system") != std::string::npos) {
            outIsSystem = true;
            outSlot = 0;
        }

        // 4. Construct friendly display name
        if (fname.find("ue4savegame") != std::string::npos && !parentName.empty()) {
            outDisplayName = parentName + "/" + fname;
        } else {
            outDisplayName = fname;
        }

        return true;
    }

    // Format destination PC save name according to user rules:
    // Episode Normal (e.g. SaveData001.sav, SaveData002.sav)
    // Episode Aigis (e.g. SaveData1001.sav, SaveData1002.sav)
    static std::string FormatPcSaveName(int targetSlot, bool isAigis, bool isSystem) {
        if (isSystem) {
            return "SystemData.sav";
        }

        char buf[64];
        if (isAigis) {
            // Episode Aigis: 4 digits (e.g. SaveData1001.sav, SaveData1002.sav)
            sprintf_s(buf, sizeof(buf), "SaveData%04d.sav", targetSlot);
        } else {
            // Episode Normal: 3 digits (e.g. SaveData001.sav, SaveData002.sav)
            sprintf_s(buf, sizeof(buf), "SaveData%03d.sav", targetSlot);
        }
        return std::string(buf);
    }
};

} // namespace Ps4Atlus
