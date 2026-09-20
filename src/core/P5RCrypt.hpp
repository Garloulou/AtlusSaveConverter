#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

#include "miniz.h"

namespace Ps4Atlus {

struct P5RMetadata {
    std::string protagonistName = "Joker";
    std::string firstName = "";
    std::string lastName = "";
    std::string groupName = "The Phantoms";
    int level = 1;
    int difficulty = 2;
    int playtimeSeconds = 0;
    std::string playtimeFormatted = "00:00:00";
    int day = 0;
    int timeOfDay = 0;
    int playthrough = 1;
    int clearCount = 0;
    bool isClear = false;
    bool isSystem = false;
    int slotNumber = 1;
};

class P5RCrypt {
public:
    using P5RMetadata = Ps4Atlus::P5RMetadata;

    // P5R PC Official AES-256 Key (base64: "3lOZS0kYSoOOtkC4c7IDfvNXnxIprUPTlUGVC3yBJF0=")
    static inline const uint8_t AES_KEY[32] = {
        0xde, 0x53, 0x99, 0x4b, 0x49, 0x18, 0x4a, 0x83,
        0x8e, 0xb6, 0x40, 0xb8, 0x73, 0xb2, 0x03, 0x7e,

        0xf3, 0x57, 0x9f, 0x12, 0x29, 0xad, 0x43, 0xd3,
        0x95, 0x41, 0x95, 0x0b, 0x7c, 0x81, 0x24, 0x5d
    };

    // CRC-32/MPEG-2 Lookup Table (256 entries)
    static inline const uint32_t CRC_TABLE[256] = {
        0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
        0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
        0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
        0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
        0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039, 0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
        0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
        0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
        0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
        0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
        0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
        0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
        0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
        0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
        0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
        0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
        0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
        0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
        0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
        0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
        0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
        0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
        0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
        0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
        0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
        0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
        0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
        0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
        0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
        0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
        0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
        0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
        0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
    };

    // Calculate CRC-32/MPEG-2 checksum
    static uint32_t CalculateCrc(const uint8_t* buffer, size_t size, uint32_t init = 0xFFFFFFFF) {
        for (size_t i = 0; i < size; ++i) {
            init = (init << 8) ^ CRC_TABLE[buffer[i] ^ (init >> 24)];
            init &= 0xFFFFFFFF;
        }
        return init;
    }

    static size_t Align(size_t v, size_t a) {
        return (v + (a - 1)) & ~(a - 1);
    }

    // AES-256-CBC Decryption via Windows BCrypt
    static bool Aes256CbcDecrypt(const uint8_t* ciphertext, size_t length, const uint8_t iv[16], std::vector<uint8_t>& plaintext) {
#ifdef _WIN32
        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_KEY_HANDLE hKey = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
        if (status != 0) return false;

        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
        if (status != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, (PUCHAR)AES_KEY, sizeof(AES_KEY), 0);
        if (status != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        uint8_t ivWork[16];
        std::memcpy(ivWork, iv, 16);

        plaintext.resize(length);
        ULONG bytesDecrypted = 0;
        status = BCryptDecrypt(hKey, (PUCHAR)ciphertext, (ULONG)length, NULL, ivWork, 16, plaintext.data(), (ULONG)plaintext.size(), &bytesDecrypted, 0);

        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);

        if (status != 0) return false;
        plaintext.resize(bytesDecrypted);
        return true;
#else
        return false;
#endif
    }

    // AES-256-CBC Encryption via Windows BCrypt
    static bool Aes256CbcEncrypt(const uint8_t* plaintext, size_t length, const uint8_t iv[16], std::vector<uint8_t>& ciphertext) {
#ifdef _WIN32
        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_KEY_HANDLE hKey = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
        if (status != 0) return false;

        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
        if (status != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, (PUCHAR)AES_KEY, sizeof(AES_KEY), 0);
        if (status != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        uint8_t ivWork[16];
        std::memcpy(ivWork, iv, 16);

        ciphertext.resize(length);
        ULONG bytesEncrypted = 0;
        status = BCryptEncrypt(hKey, (PUCHAR)plaintext, (ULONG)length, NULL, ivWork, 16, ciphertext.data(), (ULONG)ciphertext.size(), &bytesEncrypted, 0);

        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);

        if (status != 0) return false;
        ciphertext.resize(bytesEncrypted);
        return true;
#else
        return false;
#endif
    }

    // Generate random 16-byte IV
    static bool GenerateRandomIv(uint8_t iv[16]) {
#ifdef _WIN32
        NTSTATUS status = BCryptGenRandom(NULL, iv, 16, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        return status == 0;
#else
        for (int i = 0; i < 16; ++i) iv[i] = static_cast<uint8_t>(rand() & 0xFF);
        return true;
#endif
    }

    // Compress buffer using zlib (level 9)
    static bool ZlibCompress(const uint8_t* src, size_t srcLen, std::vector<uint8_t>& dest) {
        mz_ulong destLen = mz_compressBound(static_cast<mz_ulong>(srcLen));
        dest.resize(destLen);
        
        // mz_compress uses level 9 or default zlib compression
        int res = mz_compress(dest.data(), &destLen, src, static_cast<mz_ulong>(srcLen));
        if (res != MZ_OK) {
            return false;
        }
        dest.resize(destLen);
        return true;
    }

    // Decompress buffer using zlib
    static bool ZlibDecompress(const uint8_t* src, size_t srcLen, size_t expectedLen, std::vector<uint8_t>& dest) {
        dest.resize(expectedLen);
        mz_ulong destLen = static_cast<mz_ulong>(expectedLen);
        int res = mz_uncompress(dest.data(), &destLen, src, static_cast<mz_ulong>(srcLen));
        if (res != MZ_OK) {
            return false;
        }
        dest.resize(destLen);
        return true;
    }

    // Extract metadata autonomously from PS4 DATA.DAT without any param.sfo
    static bool ExtractMetadata(const std::vector<uint8_t>& data, P5RMetadata& meta) {
        if (data.size() < 0x50) return false;

        uint32_t magic = *reinterpret_cast<const uint32_t*>(data.data());
        if (magic == 0x00000002) {
            meta.isSystem = true;
            meta.protagonistName = "Données Système";
            meta.level = 1;
            meta.playtimeFormatted = "--:--:--";
            return true;
        }

        if (magic != 0x2D000000) {
            return false; // Non reconnu comme save P5R
        }

        meta.isSystem = false;

        // Big-endian header at offset 0x04..0x48
        // 0x04: 2x
        // 0x06: day (uint16 BE)
        meta.day = (static_cast<int>(data[0x06]) << 8) | data[0x07];
        // 0x08: time (uint16 BE)
        meta.timeOfDay = (static_cast<int>(data[0x08]) << 8) | data[0x09];
        // 0x0A: 2x
        // 0x0C: playtime (uint32 BE in seconds)
        meta.playtimeSeconds = (static_cast<uint32_t>(data[0x0C]) << 24) |
                               (static_cast<uint32_t>(data[0x0D]) << 16) |
                               (static_cast<uint32_t>(data[0x0E]) << 8)  |
                                static_cast<uint32_t>(data[0x0F]);

        int hours = meta.playtimeSeconds / 3600;
        int mins = (meta.playtimeSeconds % 3600) / 60;
        int secs = meta.playtimeSeconds % 60;
        char timeBuf[32];
        std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", hours, mins, secs);
        meta.playtimeFormatted = timeBuf;

        // 0x10: level (uint8)
        meta.level = data[0x10];
        // 0x11: difficulty (uint8)
        meta.difficulty = data[0x11];
        // 0x12: playthrough (uint8)
        meta.playthrough = data[0x12];
        // 0x13: clear (uint8)
        meta.clearCount = data[0x13];
        meta.isClear = (meta.clearCount > 0);

        // Scan big-endian TLV blocks starting at offset 0x50
        size_t pos = 0x50;
        while (pos + 8 <= data.size()) {
            uint32_t blockId = (static_cast<uint32_t>(data[pos]) << 24) |
                               (static_cast<uint32_t>(data[pos + 1]) << 16) |
                               (static_cast<uint32_t>(data[pos + 2]) << 8)  |
                                static_cast<uint32_t>(data[pos + 3]);

            uint32_t blockSize = (static_cast<uint32_t>(data[pos + 4]) << 24) |
                                 (static_cast<uint32_t>(data[pos + 5]) << 16) |
                                 (static_cast<uint32_t>(data[pos + 6]) << 8)  |
                                  static_cast<uint32_t>(data[pos + 7]);

            if (blockId == 0x2000) {
                break; // Sentinel block
            }

            if (blockId == 0x1001C) { // DataBlockPlayerName
                size_t dStart = pos + 8;
                if (dStart + 56 <= data.size()) {
                    // full_name_utf8 (56 bytes, null-terminated)
                    std::string fullName(reinterpret_cast<const char*>(data.data() + dStart), 56);
                    size_t nullIdx = fullName.find('\0');
                    if (nullIdx != std::string::npos) fullName.resize(nullIdx);

                    meta.protagonistName = fullName;

                    // Split into firstName / lastName on space if present
                    size_t sp = fullName.find(' ');
                    if (sp != std::string::npos) {
                        meta.firstName = fullName.substr(0, sp);
                        meta.lastName = fullName.substr(sp + 1);
                    } else {
                        meta.firstName = fullName;
                        meta.lastName = "";
                    }

                    // group_name_utf8 is at offset + 161 (56 + 20 + 20 + 40 + 25)
                    if (dStart + 161 + 37 <= data.size()) {
                        std::string grp(reinterpret_cast<const char*>(data.data() + dStart + 161), 37);
                        size_t gNull = grp.find('\0');
                        if (gNull != std::string::npos) grp.resize(gNull);
                        if (!grp.empty()) meta.groupName = grp;
                    }
                }
            }

            pos += 8 + blockSize;
        }

        return true;
    }

    // Convert PS4 decrypted save (DATA.DAT or SYSTEM.DAT) to PC format
    // Fully autonomous: NO param.sfo needed!
    static bool ConvertSaveFile(const std::string& inputPath,
                                const std::string& outputPath,
                                int targetSlot,
                                std::string& error) {
        std::error_code ec;
        if (!std::filesystem::exists(inputPath, ec)) {
            error = "Fichier source introuvable : " + inputPath;
            return false;
        }

        std::ifstream in(inputPath, std::ios::binary);
        if (!in.is_open()) {
            error = "Impossible d'ouvrir le fichier source : " + inputPath;
            return false;
        }

        in.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);

        std::vector<uint8_t> ps4Data(fileSize);
        in.read(reinterpret_cast<char*>(ps4Data.data()), fileSize);
        in.close();

        if (ps4Data.size() < 4) {
            error = "Fichier source corrompu ou trop petit.";
            return false;
        }

        uint32_t magic = *reinterpret_cast<const uint32_t*>(ps4Data.data());
        bool isSystem = (magic == 0x00000002);
        bool isGame = (magic == 0x2D000000);

        if (!isSystem && !isGame) {
            error = "Format de sauvegarde PS4 non reconnu (magic invalide).";
            return false;
        }

        P5RMetadata meta;
        meta.slotNumber = targetSlot;
        if (isGame) {
            ExtractMetadata(ps4Data, meta);
        } else {
            meta.isSystem = true;
        }

        // 1. Calculate uncompressed data CRC
        uint32_t dataCrc = CalculateCrc(ps4Data.data(), ps4Data.size());

        // 2. Prepare uncompressed Header block (0x190 bytes for Game, empty for System)
        std::vector<uint8_t> uncompHeader;
        if (isGame) {
            uncompHeader.resize(0x190, 0);

            // <IHBBBBxBBBBB64s64s256s
            *reinterpret_cast<uint32_t*>(uncompHeader.data() + 0x00) = static_cast<uint32_t>(meta.playtimeSeconds);
            *reinterpret_cast<uint16_t*>(uncompHeader.data() + 0x04) = static_cast<uint16_t>(meta.day);
            uncompHeader[0x06] = static_cast<uint8_t>(meta.timeOfDay);
            uncompHeader[0x07] = static_cast<uint8_t>(meta.playthrough);
            uncompHeader[0x08] = static_cast<uint8_t>(meta.difficulty);
            uncompHeader[0x09] = static_cast<uint8_t>(meta.level);
            uncompHeader[0x0A] = 0; // pad
            uncompHeader[0x0B] = static_cast<uint8_t>(meta.isClear ? 0xFF : 0x00);
            uncompHeader[0x0C] = 0; // fld_major
            uncompHeader[0x0D] = 0; // fld_minor
            uncompHeader[0x0E] = 0xFF; // lang0 (0xFF = PC multi)
            uncompHeader[0x0F] = 0x00; // lang1

            // 64s lname at 0x10
            std::string lname = meta.lastName.empty() ? meta.protagonistName : meta.lastName;
            size_t copyLname = (std::min)(lname.size(), size_t(63));
            std::memcpy(uncompHeader.data() + 0x10, lname.data(), copyLname);

            // 64s fname at 0x50
            std::string fname = meta.firstName;
            size_t copyFname = (std::min)(fname.size(), size_t(63));
            std::memcpy(uncompHeader.data() + 0x50, fname.data(), copyFname);

            // 256s desc at 0x90 (Autonomous description without param.sfo)
            std::ostringstream descBuilder;
            int hours = meta.playtimeSeconds / 3600;
            int mins = (meta.playtimeSeconds % 3600) / 60;
            descBuilder << "PLV:" << meta.level << " " << meta.protagonistName << "\n"
                        << "PLAY TIME:" << hours << "h " << mins << "m\n"
                        << "PHANTOMS:" << meta.groupName << "\n";
            std::string descStr = descBuilder.str();
            size_t copyDesc = (std::min)(descStr.size(), size_t(255));
            std::memcpy(uncompHeader.data() + 0x90, descStr.data(), copyDesc);
        }

        // 3. Compress Header and Data with zlib level 9
        std::vector<uint8_t> compHeader;
        uint16_t headerSize = 0x40;
        uint16_t headerSizeComp = 0;
        uint32_t saveFlags = 0x02110600;

        if (isGame && !uncompHeader.empty()) {
            headerSize = static_cast<uint16_t>(0x40 + uncompHeader.size()); // 0x1D0
            if (!ZlibCompress(uncompHeader.data(), uncompHeader.size(), compHeader)) {
                error = "Échec de la compression zlib du header.";
                return false;
            }
            saveFlags |= 1; // header compressed
            headerSizeComp = static_cast<uint16_t>(0x40 + compHeader.size());
        }

        std::vector<uint8_t> compData;
        if (!ZlibCompress(ps4Data.data(), ps4Data.size(), compData)) {
            error = "Échec de la compression zlib des données.";
            return false;
        }
        saveFlags |= 2; // data compressed

        uint32_t dataSize = static_cast<uint32_t>(ps4Data.size());
        uint32_t dataSizeComp = static_cast<uint32_t>(compData.size());

        // 4. Assemble unencrypted payload starting at offset 0x20 of the PC file
        // Header info: HHIIII 12x (32 bytes = 0x20)
        std::vector<uint8_t> payload(32, 0);
        *reinterpret_cast<uint16_t*>(payload.data() + 0x00) = headerSize;
        *reinterpret_cast<uint16_t*>(payload.data() + 0x02) = headerSizeComp;
        *reinterpret_cast<uint32_t*>(payload.data() + 0x04) = dataSize;
        *reinterpret_cast<uint32_t*>(payload.data() + 0x08) = dataSizeComp;
        *reinterpret_cast<uint32_t*>(payload.data() + 0x0C) = saveFlags;
        *reinterpret_cast<uint32_t*>(payload.data() + 0x10) = dataCrc;
        // 12 bytes padding at 0x14..0x20 remain 0

        // Append Header (aligned to 16 bytes: align(hs, 16) - 0x40)
        if (isGame && !compHeader.empty()) {
            size_t hs = headerSizeComp;
            size_t targetHeaderAligned = Align(hs, 16) - 0x40;
            size_t prevSize = payload.size();
            payload.resize(prevSize + targetHeaderAligned, 0);
            std::memcpy(payload.data() + prevSize, compHeader.data(), compHeader.size());
        }

        // Append Data (aligned to 16 bytes: align(ds, 16))
        {
            size_t ds = dataSizeComp;
            size_t targetDataAligned = Align(ds, 16);
            size_t prevSize = payload.size();
            payload.resize(prevSize + targetDataAligned, 0);
            std::memcpy(payload.data() + prevSize, compData.data(), compData.size());
        }

        // 5. Encrypt payload using AES-256-CBC
        uint8_t iv[16];
        if (!GenerateRandomIv(iv)) {
            error = "Échec de la génération de l'IV cryptographique.";
            return false;
        }

        std::vector<uint8_t> encryptedPayload;
        if (!Aes256CbcEncrypt(payload.data(), payload.size(), iv, encryptedPayload)) {
            error = "Échec du chiffrement AES-256-CBC.";
            return false;
        }

        // 6. Build final file:
        // [0x00..0x04]: "DATA"
        // [0x04..0x08]: file_crc
        // [0x08..0x0C]: timestamp
        // [0x0C..0x10]: file_flags (0x80000000 = encrypted)
        // [0x10..0x20]: file_iv (16 bytes)
        // [0x20..end]: encrypted_payload
        uint32_t fileTimestamp = static_cast<uint32_t>(std::time(nullptr));
        uint32_t fileFlags = 0x80000000;

        std::vector<uint8_t> crcPayload;
        crcPayload.resize(4 + 4 + 16 + encryptedPayload.size());
        *reinterpret_cast<uint32_t*>(crcPayload.data() + 0x00) = fileTimestamp;
        *reinterpret_cast<uint32_t*>(crcPayload.data() + 0x04) = fileFlags;
        std::memcpy(crcPayload.data() + 0x08, iv, 16);
        std::memcpy(crcPayload.data() + 0x18, encryptedPayload.data(), encryptedPayload.size());

        uint32_t fileCrc = CalculateCrc(crcPayload.data(), crcPayload.size());

        std::vector<uint8_t> finalFile;
        finalFile.reserve(4 + 4 + crcPayload.size());
        const char magicData[4] = {'D', 'A', 'T', 'A'};
        finalFile.insert(finalFile.end(), magicData, magicData + 4);
        finalFile.resize(8);
        *reinterpret_cast<uint32_t*>(finalFile.data() + 0x04) = fileCrc;
        finalFile.insert(finalFile.end(), crcPayload.begin(), crcPayload.end());

        // 7. Write to output directory
        std::filesystem::path outPath(outputPath);
        std::filesystem::create_directories(outPath.parent_path(), ec);

        std::ofstream out(outputPath, std::ios::binary);
        if (!out.is_open()) {
            error = "Impossible de créer le fichier cible : " + outputPath;
            return false;
        }

        out.write(reinterpret_cast<const char*>(finalFile.data()), finalFile.size());
        out.close();

        return true;
    }

    // Format target relative PC save path (e.g. "DATA01/DATA.DAT" or "SYSTEM/SYSTEM.DAT")
    static std::string FormatPcSavePath(int slotNumber, bool isSystem) {
        if (isSystem) return "SYSTEM/SYSTEM.DAT";
        char buf[32];
        std::snprintf(buf, sizeof(buf), "DATA%02d/DATA.DAT", slotNumber);
        return buf;
    }

    // Inspect save file to extract slot, system flag, metadata and display name
    static bool InspectSaveFile(const std::string& filePath,
                                int& outSlot,
                                bool& outIsSystem,
                                std::string& outDisplayName,
                                P5RMetadata* outMeta = nullptr) {
        std::error_code ec;
        if (!std::filesystem::exists(filePath, ec)) return false;

        std::string fname = std::filesystem::path(filePath).filename().string();
        std::string parentDir = std::filesystem::path(filePath).parent_path().filename().string();

        std::string lowerName = fname;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                       [](unsigned char c) { return (char)::tolower(c); });
        std::string lowerParent = parentDir;
        std::transform(lowerParent.begin(), lowerParent.end(), lowerParent.begin(),
                       [](unsigned char c) { return (char)::tolower(c); });

        // Read first 80 bytes to inspect magic
        std::ifstream in(filePath, std::ios::binary);
        if (!in.is_open()) return false;
        std::vector<uint8_t> headerBytes(80, 0);
        in.read(reinterpret_cast<char*>(headerBytes.data()), 80);
        size_t readCount = static_cast<size_t>(in.gcount());
        in.close();

        if (readCount < 4) return false;
        uint32_t magic = *reinterpret_cast<const uint32_t*>(headerBytes.data());

        // System save detection
        if (magic == 0x00000002 ||
            lowerName.find("system") != std::string::npos ||
            lowerParent.find("system") != std::string::npos) {
            outIsSystem = true;
            outSlot = 0;
            outDisplayName = "SYSTEM/SYSTEM.DAT (Données Système)";
            if (outMeta) {
                outMeta->isSystem = true;
                outMeta->slotNumber = 0;
                outMeta->protagonistName = "Données Système";
                outMeta->playtimeFormatted = "--:--:--";
            }
            return true;
        }

        outIsSystem = false;
        outSlot = 1;

        // Extract slot number from parent directory or filename
        std::regex slotRegex(R"((\d{1,2}))");
        std::smatch match;
        std::string combined = parentDir + " " + fname;
        if (std::regex_search(combined, match, slotRegex)) {
            try {
                int s = std::stoi(match[1].str());
                if (s >= 1 && s <= 16) outSlot = s;
            } catch (...) {}
        }

        char slotStr[32];
        std::snprintf(slotStr, sizeof(slotStr), "DATA%02d/DATA.DAT", outSlot);
        outDisplayName = slotStr;

        if (outMeta) {
            std::ifstream fullIn(filePath, std::ios::binary);
            if (fullIn.is_open()) {
                fullIn.seekg(0, std::ios::end);
                size_t sz = static_cast<size_t>(fullIn.tellg());
                fullIn.seekg(0, std::ios::beg);
                std::vector<uint8_t> fullData(sz);
                fullIn.read(reinterpret_cast<char*>(fullData.data()), sz);
                fullIn.close();

                ExtractMetadata(fullData, *outMeta);
            }
            outMeta->slotNumber = outSlot;
        }

        return true;
    }
};


} // namespace Ps4Atlus
