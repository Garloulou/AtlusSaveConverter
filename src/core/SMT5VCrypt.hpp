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

#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace Ps4Atlus {

class SMT5VCrypt {
public:
    // SMT V: Vengeance AES-256 Key: "0123456789abcdef0123456789abcdef" (32 bytes)
    static constexpr std::string_view Key = "0123456789abcdef0123456789abcdef";

    // Expected file sizes
    static constexpr size_t GameSaveSizeBytes = 449680; // 0x6DC90
    static constexpr size_t SysSaveSizeBytes  = 4832;   // 0x12E0

    // Offsets
    static constexpr size_t OffsetSha1Hash     = 0x00;  // 20 bytes
    static constexpr size_t OffsetGvasHeader   = 0x40;  // "GVAS"
    static constexpr size_t OffsetFirstName    = 0x4D8; // UTF-16LE, 16 bytes
    static constexpr size_t OffsetDifficulty   = 0x4FC; // u8 (0: Safety, 1: Casual, 2: Normal, 3: Hard)
    static constexpr size_t OffsetCycles       = 0x502; // u8
    static constexpr size_t OffsetDlcFlags     = 0x529; // u8
    static constexpr size_t OffsetPlaytimeSec  = 0x5D0; // u32
    static constexpr size_t OffsetPlayerLevel  = 0x9C8; // u16
    static constexpr size_t OffsetMacca        = 0x3D32;// u32
    static constexpr size_t OffsetGlory        = 0x3D4A;// u32
    static constexpr size_t OffsetSecondaryDlc = 0x6A07F;// u8

    struct DlcCleanResult {
        bool mainDlcCleared = false;
        uint8_t originalDlcByte = 0;
        bool secondaryDlcCleared = false;
        uint8_t originalSecondaryByte = 0;

        bool AnyCleared() const {
            return mainDlcCleared || secondaryDlcCleared;
        }
    };

    struct Smt5Metadata {
        std::string protagonistName;
        int level = 0;
        uint32_t playtimeSeconds = 0;
        std::string playtimeFormatted;
        uint32_t macca = 0;
        uint32_t glory = 0;
        uint8_t dlcByte = 0;
        int difficulty = 2; // 0: Safety, 1: Casual, 2: Normal, 3: Hard
        std::string difficultyName = "Normal";
        bool isSystem = false;
        int slotNumber = 1;
        bool isValidGvas = false;
        bool isHashValid = false;
    };

    // Calculate SHA-1 digest (20 bytes) over data buffer using Windows CNG
    static bool CalculateSha1(const uint8_t* data, size_t size, uint8_t outDigest[20]) {
        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_HASH_HANDLE hHash = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
        if (!BCRYPT_SUCCESS(status)) return false;

        DWORD cbHashObject = 0;
        DWORD cbData = 0;
        status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbHashObject, sizeof(DWORD), &cbData, 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        std::vector<uint8_t> hashObject(cbHashObject);
        status = BCryptCreateHash(hAlg, &hHash, hashObject.data(), cbHashObject, NULL, 0, 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        status = BCryptHashData(hHash, (PUCHAR)data, static_cast<ULONG>(size), 0);
        if (BCRYPT_SUCCESS(status)) {
            status = BCryptFinishHash(hHash, (PUCHAR)outDigest, 20, 0);
        }

        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return BCRYPT_SUCCESS(status);
    }

    // Encrypt in-memory buffer using AES-256-ECB (Windows CNG)
    static bool EncryptAes256Ecb(const uint8_t* inData, size_t size, std::vector<uint8_t>& outData) {
        if (size == 0 || size % 16 != 0) return false;

        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_KEY_HANDLE hKey = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
        if (!BCRYPT_SUCCESS(status)) return false;

        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        DWORD cbKeyObject = 0;
        DWORD cbData = 0;
        status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbKeyObject, sizeof(DWORD), &cbData, 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        std::vector<uint8_t> keyObject(cbKeyObject);
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, keyObject.data(), cbKeyObject,
                                            (PUCHAR)Key.data(), static_cast<ULONG>(Key.size()), 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        outData.resize(size);
        ULONG cbResult = 0;
        status = BCryptEncrypt(hKey, (PUCHAR)inData, static_cast<ULONG>(size), NULL, NULL, 0,
                               outData.data(), static_cast<ULONG>(size), &cbResult, 0);

        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return BCRYPT_SUCCESS(status);
    }

    // Decrypt in-memory buffer using AES-256-ECB (Windows CNG)
    static bool DecryptAes256Ecb(const uint8_t* inData, size_t size, std::vector<uint8_t>& outData) {
        if (size == 0 || size % 16 != 0) return false;

        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_KEY_HANDLE hKey = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
        if (!BCRYPT_SUCCESS(status)) return false;

        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        DWORD cbKeyObject = 0;
        DWORD cbData = 0;
        status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbKeyObject, sizeof(DWORD), &cbData, 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        std::vector<uint8_t> keyObject(cbKeyObject);
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, keyObject.data(), cbKeyObject,
                                            (PUCHAR)Key.data(), static_cast<ULONG>(Key.size()), 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        outData.resize(size);
        ULONG cbResult = 0;
        status = BCryptDecrypt(hKey, (PUCHAR)inData, static_cast<ULONG>(size), NULL, NULL, 0,
                               outData.data(), static_cast<ULONG>(size), &cbResult, 0);

        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return BCRYPT_SUCCESS(status);
    }

    // Check if buffer is unencrypted SMT5V save data (starts with GVAS at offset 0x40)
    static bool IsDecrypted(const std::vector<uint8_t>& buf) {
        if (buf.size() < OffsetGvasHeader + 4) return false;
        return (buf[OffsetGvasHeader + 0] == 'G' &&
                buf[OffsetGvasHeader + 1] == 'V' &&
                buf[OffsetGvasHeader + 2] == 'A' &&
                buf[OffsetGvasHeader + 3] == 'S');
    }

    // Validate SHA-1 checksum in header (0x00..0x14) against calculated SHA-1 on buf[0x40..EOF]
    static bool ValidateSha1(const std::vector<uint8_t>& buf) {
        if (buf.size() < OffsetGvasHeader) return false;
        uint8_t calcHash[20] = {0};
        if (!CalculateSha1(buf.data() + OffsetGvasHeader, buf.size() - OffsetGvasHeader, calcHash)) {
            return false;
        }
        return (std::memcmp(buf.data() + OffsetSha1Hash, calcHash, 20) == 0);
    }

    // Update SHA-1 checksum at 0x00..0x14 based on data at buf[0x40..EOF]
    static bool UpdateSha1(std::vector<uint8_t>& buf) {
        if (buf.size() < OffsetGvasHeader) return false;
        uint8_t calcHash[20] = {0};
        if (!CalculateSha1(buf.data() + OffsetGvasHeader, buf.size() - OffsetGvasHeader, calcHash)) {
            return false;
        }
        std::memcpy(buf.data() + OffsetSha1Hash, calcHash, 20);
        return true;
    }

    // Clean DLC flags: clears byte at 0x529 and 0x6A07F, then recalculates SHA-1
    static DlcCleanResult CleanDlcFlags(std::vector<uint8_t>& buf) {
        DlcCleanResult res;
        if (!IsDecrypted(buf)) return res;

        if (buf.size() > OffsetDlcFlags) {
            res.originalDlcByte = buf[OffsetDlcFlags];
            if (buf[OffsetDlcFlags] != 0x00) {
                buf[OffsetDlcFlags] = 0x00;
                res.mainDlcCleared = true;
            }
        }

        if (buf.size() > OffsetSecondaryDlc) {
            res.originalSecondaryByte = buf[OffsetSecondaryDlc];
            if (buf[OffsetSecondaryDlc] != 0x00) {
                buf[OffsetSecondaryDlc] = 0x00;
                res.secondaryDlcCleared = true;
            }
        }

        if (res.AnyCleared()) {
            UpdateSha1(buf);
        }

        return res;
    }

    // Extract metadata from unencrypted buffer
    static Smt5Metadata ExtractMetadata(const std::vector<uint8_t>& buf) {
        Smt5Metadata meta;
        if (!IsDecrypted(buf)) return meta;

        meta.isValidGvas = true;
        meta.isHashValid = ValidateSha1(buf);
        meta.isSystem = (buf.size() < 100000); // SysSave is 4832 bytes

        if (!meta.isSystem && buf.size() >= GameSaveSizeBytes) {
            // Protagonist name (UTF-16LE at 0x4D8)
            std::u16string u16name;
            for (size_t i = OffsetFirstName; i + 1 < OffsetFirstName + 32 && i + 1 < buf.size(); i += 2) {
                char16_t ch = static_cast<char16_t>(buf[i] | (buf[i + 1] << 8));
                if (ch == 0) break;
                u16name.push_back(ch);
            }
            std::string nameUtf8;
            for (char16_t c : u16name) {
                if (c < 128) nameUtf8.push_back(static_cast<char>(c));
                else nameUtf8.push_back('?');
            }
            meta.protagonistName = nameUtf8;

            // Player level (u16 at 0x9C8)
            if (buf.size() >= OffsetPlayerLevel + 2) {
                meta.level = static_cast<int>(buf[OffsetPlayerLevel] | (buf[OffsetPlayerLevel + 1] << 8));
            }

            // Playtime seconds (u32 at 0x5D0)
            if (buf.size() >= OffsetPlaytimeSec + 4) {
                meta.playtimeSeconds = static_cast<uint32_t>(buf[OffsetPlaytimeSec] |
                                                            (buf[OffsetPlaytimeSec + 1] << 8) |
                                                            (buf[OffsetPlaytimeSec + 2] << 16) |
                                                            (buf[OffsetPlaytimeSec + 3] << 24));
                uint32_t hours = meta.playtimeSeconds / 3600;
                uint32_t mins  = (meta.playtimeSeconds % 3600) / 60;
                meta.playtimeFormatted = std::to_string(hours) + "h " + (mins < 10 ? "0" : "") + std::to_string(mins) + "m";
            }

            // Macca (u32 at 0x3D32)
            if (buf.size() >= OffsetMacca + 4) {
                meta.macca = static_cast<uint32_t>(buf[OffsetMacca] |
                                                   (buf[OffsetMacca + 1] << 8) |
                                                   (buf[OffsetMacca + 2] << 16) |
                                                   (buf[OffsetMacca + 3] << 24));
            }

            // Glory (u32 at 0x3D4A)
            if (buf.size() >= OffsetGlory + 4) {
                meta.glory = static_cast<uint32_t>(buf[OffsetGlory] |
                                                   (buf[OffsetGlory + 1] << 8) |
                                                   (buf[OffsetGlory + 2] << 16) |
                                                   (buf[OffsetGlory + 3] << 24));
            }

            // DLC byte (u8 at 0x529)
            if (buf.size() >= OffsetDlcFlags + 1) {
                meta.dlcByte = buf[OffsetDlcFlags];
            }

            // Difficulty (u8 at 0x4FC)
            if (buf.size() >= OffsetDifficulty + 1) {
                meta.difficulty = buf[OffsetDifficulty];
                switch (meta.difficulty) {
                    case 0: meta.difficultyName = "Safety"; break;
                    case 1: meta.difficultyName = "Casual"; break;
                    case 2: meta.difficultyName = "Normal"; break;
                    case 3: meta.difficultyName = "Hard"; break;
                    default: meta.difficultyName = "Normal"; break;
                }
            }
        }

        return meta;
    }

    // Format PC Save file name according to slot and system status
    static std::string FormatPcSaveName(int slot, bool isSystem) {
        if (isSystem) {
            return "SysSave.sav";
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "GameSave%02d.sav", slot);
        return std::string(buf);
    }

    // Inspect save file from disk, reading metadata, detecting slot, and checking headers
    static bool InspectSaveFile(const std::string& filePath, int& outSlot, bool& outIsSystem, std::string& outDisplayName, Smt5Metadata* outMeta = nullptr) {
        outSlot = 1;
        outIsSystem = false;
        outDisplayName = std::filesystem::path(filePath).filename().string();

        std::filesystem::path p(filePath);
        std::string filename = p.filename().string();
        std::string parentDir = p.parent_path().filename().string();

        // 1. Detect slot from directory or file name
        // Example: dec_GameSave01_CUSA42698, dec_SysSave_CUSA42698, GameSave02.sav
        std::string combined = parentDir + " " + filename;
        std::transform(combined.begin(), combined.end(), combined.begin(), [](unsigned char c) { return (char)::tolower(c); });

        if (combined.find("syssave") != std::string::npos || combined.find("system") != std::string::npos) {
            outIsSystem = true;
            outSlot = 0;
            outDisplayName = "Paramètres Système (SysSave)";
        } else {
            // Look for GameSaveXX or SaveDataXX
            std::regex slotRegex(R"((?:gamesave|savedata|save|slot)[_-]?(\d{1,4}))", std::regex_constants::icase);
            std::smatch match;
            if (std::regex_search(combined, match, slotRegex) && match.size() > 1) {
                outSlot = std::stoi(match[1].str());
            }
            char disp[64];
            std::snprintf(disp, sizeof(disp), "Emplacement %02d (GameSave%02d)", outSlot, outSlot);
            outDisplayName = disp;
        }

        // 2. Read file to inspect binary headers
        std::ifstream f(filePath, std::ios::binary);
        if (!f) return false;

        std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        if (buf.size() < 68) return false;

        std::vector<uint8_t> decBuf;
        if (IsDecrypted(buf)) {
            decBuf = buf;
        } else {
            // Try decrypting with AES-256-ECB key
            if (DecryptAes256Ecb(buf.data(), buf.size(), decBuf)) {
                if (!IsDecrypted(decBuf)) {
                    decBuf.clear();
                }
            }
        }

        if (!decBuf.empty()) {
            Smt5Metadata meta = ExtractMetadata(decBuf);
            if (outMeta) *outMeta = meta;

            if (!meta.isSystem && !meta.protagonistName.empty()) {
                char disp[128];
                std::snprintf(disp, sizeof(disp), "%s (Niv. %d) - Emplacement %02d [%s]",
                              meta.protagonistName.c_str(), meta.level, outSlot, meta.playtimeFormatted.c_str());
                outDisplayName = disp;
            }
            return true;
        }

        return true;
    }

    // Process and encrypt save file for PC Steam
    static bool EncryptSaveFile(const std::string& inputPath, const std::string& outputPath,
                               bool removeDlcFlags, DlcCleanResult& dlcResult, std::string& errorMsg) {
        std::ifstream in(inputPath, std::ios::binary);
        if (!in) {
            errorMsg = "Impossible d'ouvrir le fichier source : " + inputPath;
            return false;
        }

        std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        if (buf.empty()) {
            errorMsg = "Le fichier source est vide.";
            return false;
        }

        // Pad to 16 bytes if necessary for AES block cipher
        if (buf.size() % 16 != 0) {
            size_t paddedSize = ((buf.size() + 15) / 16) * 16;
            buf.resize(paddedSize, 0x00);
        }

        std::vector<uint8_t> workingBuf;
        if (IsDecrypted(buf)) {
            workingBuf = std::move(buf);
        } else {
            // Try decrypting first
            if (!DecryptAes256Ecb(buf.data(), buf.size(), workingBuf) || !IsDecrypted(workingBuf)) {
                errorMsg = "Le fichier n'a pas un en-tête GVAS valide (déchiffré ou chiffré AES).";
                return false;
            }
        }

        // Clean DLC flags if requested
        if (removeDlcFlags) {
            dlcResult = CleanDlcFlags(workingBuf);
        }

        // Always ensure SHA-1 at 0x00..0x14 is updated and valid
        if (!UpdateSha1(workingBuf)) {
            errorMsg = "Échec du calcul du condensat SHA-1.";
            return false;
        }

        // Encrypt using AES-256-ECB
        std::vector<uint8_t> encryptedBuf;
        if (!EncryptAes256Ecb(workingBuf.data(), workingBuf.size(), encryptedBuf)) {
            errorMsg = "Échec du chiffrement AES-256-ECB.";
            return false;
        }

        // Write to destination
        std::ofstream out(outputPath, std::ios::binary);
        if (!out) {
            errorMsg = "Impossible d'écrire le fichier de destination : " + outputPath;
            return false;
        }

        out.write(reinterpret_cast<const char*>(encryptedBuf.data()), encryptedBuf.size());
        out.close();

        return true;
    }
};

} // namespace Ps4Atlus
