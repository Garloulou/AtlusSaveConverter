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
#include <chrono>
#include <ctime>

#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace Ps4Atlus {

class P3PCrypt {
public:
    // Sizes
    static constexpr size_t PS4GameSaveSizeBytes = 91947;
    static constexpr size_t PcGameSaveSizeBytes  = 91932;
    static constexpr size_t SysSaveSizeBytes     = 192;
    static constexpr size_t BinSlotSizeBytes     = 1340;

    // Chunk Offsets in PS4 GAME.BIN
    static constexpr size_t OffsetNameChunk      = 0x04;  // Chunk 1 (starts at 0x04, size 0x24 = 36 bytes)
    static constexpr size_t OffsetStatsChunk     = 0x30;  // Chunk 2 (starts at 0x30, size 0x50 = 80 bytes)
    static constexpr size_t OffsetChunk39PS4     = 0x5158;// Chunk 39 on PS4 (size 3072) -> Chunk 38 on PC
    static constexpr size_t OffsetChunk38PS4Extra= 0x5D60;// 15-byte extra Chunk 38 on PS4 to remove

    // Offsets in .BINslot
    static constexpr size_t OffsetSlotId         = 0x00;  // 8 bytes: "SAVE%04d"
    static constexpr size_t OffsetSlotSig        = 0x08;  // 16 bytes: fc 02 2a ce 89 27 b1 af e2 a8 01 06 5d 91 60 21
    static constexpr size_t OffsetSlotMd5        = 0x18;  // 16 bytes: MD5 hash of P3PSAVE%04d.BIN
    static constexpr size_t OffsetSlotTitle      = 0x28;  // "Persona3 PORTABLE\0"
    static constexpr size_t OffsetSlotHeader     = 0xA8;  // "SAVE DATA\0"
    static constexpr size_t OffsetSlotText       = 0x100; // Text summary block (1024 bytes)
    static constexpr size_t OffsetSlotTimestamp  = 0x528; // 12 bytes: u16 year, month, day, hour, min, sec

    struct P3PMetadata {
        std::string protagonistName;
        std::string firstName;
        std::string lastName;
        int level = 1;
        int gender = 0; // 0 = Male (Makoto), 1 = Female (Kotone)
        std::string genderStr = "Masculin";
        int maxHp = 0;
        int maxSp = 0;
        uint32_t playtimeSeconds = 0;
        uint32_t playtimeHours = 0;
        uint32_t playtimeMinutes = 0;
        std::string playtimeFormatted = "0h 00m";
        std::string inGameDate = "04/07";
        int inGameMonth = 4;
        int inGameDay = 7;
        int timePhase = 5;
        int slotNumber = 1;
        bool isClear = false;
        int clearCount = 0;
        bool isSystem = false;
        size_t fileSize = 0;
        std::string md5Checksum;
    };

    // Native MD5 calculation via Windows CNG (BCrypt)
    // Native MD5 calculation via Windows CNG (BCrypt) with optional salt appended
    static bool CalculateMd5WithSalt(const uint8_t* data, size_t size, const char* salt, size_t saltLen, uint8_t outDigest[16]) {
        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_HASH_HANDLE hHash = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, NULL, 0);
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
        if (BCRYPT_SUCCESS(status) && salt && saltLen > 0) {
            status = BCryptHashData(hHash, (PUCHAR)salt, static_cast<ULONG>(saltLen), 0);
        }
        if (BCRYPT_SUCCESS(status)) {
            status = BCryptFinishHash(hHash, (PUCHAR)outDigest, 16, 0);
        }

        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return BCRYPT_SUCCESS(status);
    }

    static bool CalculateMd5(const uint8_t* data, size_t size, uint8_t outDigest[16]) {
        return CalculateMd5WithSalt(data, size, nullptr, 0, outDigest);
    }

    // Native SHA-1 calculation for Steam remotecache.vdf
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

    // Helper: Convert MD5 bytes to 32-char hex string
    static std::string DigestToHex(const uint8_t digest[16]) {
        std::ostringstream oss;
        for (int i = 0; i < 16; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
        }
        return oss.str();
    }

    // Decode Atlus custom 2-byte font table or ASCII protagonist name
    static std::string DecodeAtlusNamePart(const uint8_t* bytes, size_t len) {
        if (!bytes || len == 0) return "";

        // Skip leading nulls if any
        size_t startOffset = 0;
        while (startOffset < len && bytes[startOffset] == 0) {
            startOffset++;
        }
        if (startOffset >= len) return "";

        const uint8_t* actualBytes = &bytes[startOffset];
        size_t actualLen = len - startOffset;

        // Check if pure ASCII
        bool isAscii = true;
        for (size_t i = 0; i < actualLen; ++i) {
            if (actualBytes[i] != 0 && (actualBytes[i] < 32 || actualBytes[i] > 126)) {
                isAscii = false;
                break;
            }
        }

        if (isAscii) {
            std::string s(reinterpret_cast<const char*>(actualBytes), actualLen);
            auto nul = s.find('\0');
            if (nul != std::string::npos) s.resize(nul);
            // Trim spaces
            size_t start = s.find_first_not_of(" \t\r\n");
            size_t end = s.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) return "";
            return s.substr(start, end - start + 1);
        }

        // Atlus 2-byte encoding: 0x80 [value]
        std::string res;
        size_t i = 0;
        while (i < actualLen) {
            if (actualBytes[i] == 0) {
                // If double null or at end, finish
                if (i + 1 >= actualLen || actualBytes[i + 1] == 0) break;
                i++;
                continue;
            }
            if (actualBytes[i] == 0x80 && i + 1 < actualLen) {
                uint8_t val = actualBytes[i + 1];
                if (val >= 0xa1 && val <= 0xba) {
                    res.push_back(static_cast<char>('A' + (val - 0xa1)));
                } else if (val >= 0xc1 && val <= 0xda) {
                    res.push_back(static_cast<char>('a' + (val - 0xc1)));
                } else if (val >= 0x30 && val <= 0x39) {
                    res.push_back(static_cast<char>('0' + (val - 0x30)));
                } else {
                    res.push_back('?');
                }
                i += 2;
            } else {
                if (actualBytes[i] >= 32 && actualBytes[i] <= 126) {
                    res.push_back(static_cast<char>(actualBytes[i]));
                }
                i += 1;
            }
        }

        // Trim spaces
        size_t start = res.find_first_not_of(" \t\r\n");
        size_t end = res.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return res.substr(start, end - start + 1);
    }

    struct SaveChunk {
        uint32_t id;
        const uint8_t* payload;
        size_t size;
    };

    static std::vector<SaveChunk> ParseChunks(const uint8_t* data, size_t size) {
        std::vector<SaveChunk> chunks;
        if (size < 12) return chunks;
        size_t offset = 4; // skip 4-byte magic
        while (offset + 8 <= size) {
            uint32_t cid = *reinterpret_cast<const uint32_t*>(&data[offset]);
            uint32_t csz = *reinterpret_cast<const uint32_t*>(&data[offset + 4]);
            if (cid == 0xFFFFFFFF) break;
            if (offset + 8 + csz > size) break;
            chunks.push_back({ cid, &data[offset + 8], static_cast<size_t>(csz) });
            offset += 8 + csz;
        }
        return chunks;
    }

    static const SaveChunk* FindChunk(const std::vector<SaveChunk>& chunks, uint32_t id) {
        for (const auto& c : chunks) {
            if (c.id == id) return &c;
        }
        return nullptr;
    }

    // Convert in-game day counter to Month/Day (April 1 = Day 0 -> April 7 = Day 6, Day 337 = March 4)
    static void CalcDateFromDay(int dayNum, int& outMonth, int& outDay) {
        struct MonthInfo { int month; int days; };
        static const MonthInfo months[] = {
            { 4, 30 }, // April
            { 5, 31 }, // May
            { 6, 30 }, // June
            { 7, 31 }, // July
            { 8, 31 }, // August
            { 9, 30 }, // September
            { 10, 31 }, // October
            { 11, 30 }, // November
            { 12, 31 }, // December
            { 1, 31 }, // January
            { 2, 28 }, // February
            { 3, 31 }  // March
        };
        int cur = (dayNum < 0) ? 0 : dayNum;
        for (const auto& m : months) {
            if (cur < m.days) {
                outMonth = m.month;
                outDay = cur + 1;
                return;
            }
            cur -= m.days;
        }
        outMonth = 3;
        outDay = 31;
    }

    // Extract all save metadata from binary chunks
    static void ExtractP3PMetadata(const uint8_t* data, size_t size, P3PMetadata& meta) {
        auto chunks = ParseChunks(data, size);

        // 1. Name from Chunk 1 (id=1, 36 bytes: 18 last, 18 first)
        const SaveChunk* c1 = FindChunk(chunks, 1);
        if (c1 && c1->size >= 36) {
            meta.lastName = DecodeAtlusNamePart(&c1->payload[0], 18);
            meta.firstName = DecodeAtlusNamePart(&c1->payload[18], 18);
        }

        // For clear saves / new game+, if Chunk 1 is "Makoto Yuki" or empty, Chunk 37 retains the clear protagonist name
        const SaveChunk* c37 = FindChunk(chunks, 37);
        if (c37 && c37->size >= 36) {
            std::string c37Last = DecodeAtlusNamePart(&c37->payload[4], 16);
            std::string c37First = DecodeAtlusNamePart(&c37->payload[20], 16);
            if (!c37Last.empty() && (meta.lastName.empty() || meta.lastName == "Yuki")) {
                meta.lastName = c37Last;
                if (!c37First.empty()) meta.firstName = c37First;
            }
        }

        if (!meta.firstName.empty() && !meta.lastName.empty()) {
            meta.protagonistName = meta.firstName + " " + meta.lastName;
        } else if (!meta.firstName.empty()) {
            meta.protagonistName = meta.firstName;
        } else if (!meta.lastName.empty()) {
            meta.protagonistName = meta.lastName;
        } else {
            meta.protagonistName = "Protagoniste";
        }

        // 2. Gender from Chunk 5 (id=5) offset 1: 0 = Male (Makoto), 1 = Female (Kotone)
        const SaveChunk* c5 = FindChunk(chunks, 5);
        if (c5 && c5->size >= 2) {
            meta.gender = c5->payload[1];
        } else {
            meta.gender = 0;
        }
        meta.genderStr = (meta.gender == 1) ? "Féminin (Kotone)" : "Masculin (Makoto)";

        // 3. Stats & Level from Chunk 2 (id=2, 80 bytes)
        const SaveChunk* c2 = FindChunk(chunks, 2);
        if (c2 && c2->size >= 12) {
            meta.level = *reinterpret_cast<const uint16_t*>(&c2->payload[6]);
            meta.maxHp = *reinterpret_cast<const uint16_t*>(&c2->payload[8]);
            meta.maxSp = *reinterpret_cast<const uint16_t*>(&c2->payload[10]);
        }

        // 4. Calendar Date from Chunk 8 (id=8, uint16 dayCount) and Phase from Chunk 9 (id=9, uint8)
        int month = 4, day = 7;
        int timePhase = 5;
        const SaveChunk* c8 = FindChunk(chunks, 8);
        if (c8 && c8->size >= 2) {
            uint16_t dayCount = *reinterpret_cast<const uint16_t*>(c8->payload);
            CalcDateFromDay(dayCount, month, day);
        }
        const SaveChunk* c9 = FindChunk(chunks, 9);
        if (c9 && c9->size >= 1) {
            timePhase = c9->payload[0];
        }
        meta.inGameMonth = month;
        meta.inGameDay = day;
        meta.timePhase = timePhase;
        char dateBuf[16];
        std::snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d", month, day);
        meta.inGameDate = dateBuf;

        // 5. Playtime from Chunk 25 (id=25) offset 0xbc (frames @ 30fps)
        const SaveChunk* c25 = FindChunk(chunks, 25);
        if (c25 && c25->size >= 0xc0) {
            uint32_t frames = *reinterpret_cast<const uint32_t*>(&c25->payload[0xbc]);
            uint32_t totalSec = frames / 30;
            meta.playtimeSeconds = totalSec;
            uint32_t h = totalSec / 3600;
            uint32_t m = (totalSec % 3600) / 60;
            meta.playtimeHours = h;
            meta.playtimeMinutes = m;
            char timeBuf[32];
            std::snprintf(timeBuf, sizeof(timeBuf), "%uh %02um", h, m);
            meta.playtimeFormatted = timeBuf;
        }

        // 6. Clear Data detection from Chunk 10 (id=10, state: 0 = clear, 1 = active) and Chunk 35 (id=35, clear count)
        const SaveChunk* c10 = FindChunk(chunks, 10);
        const SaveChunk* c35 = FindChunk(chunks, 35);
        if (c35 && c35->size >= 4) {
            meta.clearCount = static_cast<int>(*reinterpret_cast<const uint32_t*>(c35->payload));
        }
        if (c10 && c10->size >= 4) {
            uint32_t gameState = *reinterpret_cast<const uint32_t*>(c10->payload);
            if (gameState == 0 && meta.clearCount > 0) {
                meta.isClear = true;
            }
        }
    }

    // Inspect save file and extract metadata
    static bool InspectSaveFile(const std::string& filePath,
                               int& outSlot,
                               bool& outIsSystem,
                               std::string& outDisplayName,
                               P3PMetadata* outMeta = nullptr) {
        std::error_code ec;
        if (!std::filesystem::exists(filePath, ec)) return false;

        uintmax_t fsize = std::filesystem::file_size(filePath, ec);
        std::string fname = std::filesystem::path(filePath).filename().string();
        std::string parent = std::filesystem::path(filePath).parent_path().filename().string();

        std::string lowerName = fname;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return (char)::tolower(c); });
        std::string lowerParent = parent;
        std::transform(lowerParent.begin(), lowerParent.end(), lowerParent.begin(), [](unsigned char c) { return (char)::tolower(c); });

        // System save detection
        if (lowerName.find("sys") != std::string::npos ||
            lowerParent.find("sys") != std::string::npos ||
            fsize == SysSaveSizeBytes) {
            outIsSystem = true;
            outSlot = 0;
            outDisplayName = "SYSTEM.BIN";
            if (outMeta) {
                outMeta->isSystem = true;
                outMeta->slotNumber = 0;
                outMeta->protagonistName = "Système";
                outMeta->fileSize = fsize;
            }
            return true;
        }

        // Slot number extraction
        outIsSystem = false;
        outSlot = 1;
        std::string combined = parent + " " + fname;
        std::regex slotRegex(R"((\d{1,4}))");
        std::smatch match;
        if (std::regex_search(combined, match, slotRegex)) {
            outSlot = std::stoi(match[1].str());
        }

        char buf[64];
        std::snprintf(buf, sizeof(buf), "P3PSAVE%04d.BIN", outSlot);
        outDisplayName = buf;

        if (outMeta) {
            outMeta->slotNumber = outSlot;
            outMeta->isSystem = false;
            outMeta->fileSize = fsize;
        }

        // Open and inspect binary content
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return true;

        std::vector<uint8_t> data(static_cast<size_t>(fsize));
        file.read(reinterpret_cast<char*>(data.data()), fsize);
        file.close();

        if (outMeta) {
            ExtractP3PMetadata(data.data(), data.size(), *outMeta);
            outMeta->slotNumber = outSlot;
            outMeta->isSystem = false;
            outMeta->fileSize = fsize;
        }

        return true;
    }

    // Convert PS4 save to PC format & generate .BINslot
    static bool ConvertSaveFile(const std::string& inputPath,
                                const std::string& outputBinPath,
                                const std::string& outputBinSlotPath,
                                int slotNumber,
                                std::string& error) {
        std::ifstream inFile(inputPath, std::ios::binary);
        if (!inFile.is_open()) {
            error = "Impossible d'ouvrir le fichier source : " + inputPath;
            return false;
        }

        inFile.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(fileSize);
        inFile.read(reinterpret_cast<char*>(data.data()), fileSize);
        inFile.close();

        // 1. System Save
        if (fileSize == SysSaveSizeBytes) {
            std::ofstream outFile(outputBinPath, std::ios::binary);
            if (!outFile.is_open()) {
                error = "Impossible de créer SYSTEM.BIN : " + outputBinPath;
                return false;
            }
            outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
            outFile.close();
            return true;
        }

        // 2. Game Save
        if (fileSize == PS4GameSaveSizeBytes) {
            // Validate Chunk 39 at 0x5158 (id=39, size=3072)
            uint32_t chunk39Id = *reinterpret_cast<const uint32_t*>(&data[OffsetChunk39PS4]);
            uint32_t chunk39Size = *reinterpret_cast<const uint32_t*>(&data[OffsetChunk39PS4 + 4]);

            if (chunk39Id != 39 || chunk39Size != 3072) {
                error = "Format de sauvegarde PS4 inattendu au Chunk 39 (offset 0x5158).";
                return false;
            }

            // Validate Chunk 38 at 0x5D60 (id=38, size=7)
            uint32_t chunk38Id = *reinterpret_cast<const uint32_t*>(&data[OffsetChunk38PS4Extra]);
            uint32_t chunk38Size = *reinterpret_cast<const uint32_t*>(&data[OffsetChunk38PS4Extra + 4]);

            if (chunk38Id != 38 || chunk38Size != 7) {
                error = "Format de sauvegarde PS4 inattendu au Chunk 38 PS4 (offset 0x5D60).";
                return false;
            }

            // Patch 1: Rename Chunk 39 to Chunk 38 at 0x5158
            *reinterpret_cast<uint32_t*>(&data[OffsetChunk39PS4]) = 38;

            // Patch 2: Strip the 15-byte PS4-specific Chunk 38 at 0x5D60
            data.erase(data.begin() + OffsetChunk38PS4Extra, data.begin() + OffsetChunk38PS4Extra + 15);

            if (data.size() != PcGameSaveSizeBytes) {
                error = "Taille incohérente après conversion PC : " + std::to_string(data.size()) + " (attendu: 91932).";
                return false;
            }
        } else if (fileSize == PcGameSaveSizeBytes) {
            // Already PC format, keep as is
        } else {
            error = "Taille de fichier inconnue pour P3P : " + std::to_string(fileSize) + " octets.";
            return false;
        }

        // Write converted P3PSAVE%04d.BIN
        std::ofstream outBin(outputBinPath, std::ios::binary);
        if (!outBin.is_open()) {
            error = "Impossible de créer le fichier PC .BIN : " + outputBinPath;
            return false;
        }
        outBin.write(reinterpret_cast<const char*>(data.data()), data.size());
        outBin.close();

        // Calculate MD5 of the converted .BIN file
        uint8_t md5Digest[16] = {0};
        if (!CalculateMd5(data.data(), data.size(), md5Digest)) {
            error = "Échec du calcul MD5 sur le fichier converti.";
            return false;
        }

        // Build .BINslot companion file (1340 bytes)
        std::vector<uint8_t> slotData(GetBinSlotTemplate(), GetBinSlotTemplate() + BinSlotSizeBytes);

        // Update slot string at 0x00..0x08
        char slotStr[16];
        std::snprintf(slotStr, sizeof(slotStr), "SAVE%04d", slotNumber);
        std::memcpy(&slotData[OffsetSlotId], slotStr, 8);

        // Insert MD5 hash of .BIN at 0x18..0x28
        std::memcpy(&slotData[OffsetSlotMd5], md5Digest, 16);

        // Extract real metadata from save data for .BINslot text block
        P3PMetadata meta;
        ExtractP3PMetadata(data.data(), data.size(), meta);

        // Format Shift-JIS text block at 0x128 with real save metadata
        char textBuf[1024];
        std::snprintf(textBuf, sizeof(textBuf),
            "%s"
            "\x93\xfa\x95\x74:%02d/%02d:%d\n"
            "\x96\xbc\x91\x4f:%s Lv:%d\n"
            "\x90\xab\x95\xca:%d\n"
            "\x8f\xea\x8f\x8a:7/2\n"
            "\x83\x76\x83\x8c\x83\x43\x8e\x9e\x8a\xd4%u:%02u\n"
            "\x93\xef\x88\xd5\x93\x78:2\n"
            "LANG5\n"
            "\x83\x4e\x83\x8a\x83\x41\x89\xf1\x90\x94:%d",
            (meta.isClear ? "CLEAR DATA\n" : ""),
            meta.inGameMonth, meta.inGameDay, meta.timePhase,
            meta.protagonistName.c_str(), meta.level,
            meta.gender,
            meta.playtimeHours, meta.playtimeMinutes,
            (meta.isClear ? (meta.clearCount > 0 ? meta.clearCount : 1) : 0)
        );

        // Clear text region from 0x128 to 0x524 and copy formatted text
        std::memset(&slotData[0x128], 0, 0x524 - 0x128);
        size_t textLen = std::strlen(textBuf);
        if (textLen > (0x524 - 0x128)) textLen = 0x524 - 0x128;
        std::memcpy(&slotData[0x128], textBuf, textLen);

        // Update timestamp at 0x528
        auto now = std::time(nullptr);
        struct tm tmNow;
        localtime_s(&tmNow, &now);

        uint16_t year = static_cast<uint16_t>(tmNow.tm_year + 1900);
        uint16_t monthVal = static_cast<uint16_t>(tmNow.tm_mon + 1);
        uint16_t dayVal = static_cast<uint16_t>(tmNow.tm_mday);
        uint16_t hour = static_cast<uint16_t>(tmNow.tm_hour);
        uint16_t minute = static_cast<uint16_t>(tmNow.tm_min);
        uint16_t second = static_cast<uint16_t>(tmNow.tm_sec);

        *reinterpret_cast<uint16_t*>(&slotData[OffsetSlotTimestamp + 0]) = year;
        *reinterpret_cast<uint16_t*>(&slotData[OffsetSlotTimestamp + 2]) = monthVal;
        *reinterpret_cast<uint16_t*>(&slotData[OffsetSlotTimestamp + 4]) = dayVal;
        *reinterpret_cast<uint16_t*>(&slotData[OffsetSlotTimestamp + 6]) = hour;
        *reinterpret_cast<uint16_t*>(&slotData[OffsetSlotTimestamp + 8]) = minute;
        *reinterpret_cast<uint16_t*>(&slotData[OffsetSlotTimestamp + 10]) = second;

        // Calculate and inject header signature at 0x08..0x18:
        // MD5( slotData[0x28:1340] + "P3POTABL" )
        uint8_t slotSig[16] = {0};
        static constexpr const char SaltP3P[] = "P3POTABL";
        if (!CalculateMd5WithSalt(&slotData[OffsetSlotTitle], BinSlotSizeBytes - OffsetSlotTitle, SaltP3P, 8, slotSig)) {
            error = "Échec du calcul de la signature de vérification du .BINslot.";
            return false;
        }
        std::memcpy(&slotData[OffsetSlotSig], slotSig, 16);

        // Write .BINslot
        std::ofstream outSlot(outputBinSlotPath, std::ios::binary);
        if (!outSlot.is_open()) {
            error = "Impossible de créer le fichier compagnon .BINslot : " + outputBinSlotPath;
            return false;
        }
        outSlot.write(reinterpret_cast<const char*>(slotData.data()), slotData.size());
        outSlot.close();

        return true;
    }

    // Synchronize Steam remotecache.vdf if Steam directory is detected
    static bool UpdateSteamRemoteCache(const std::string& remoteDirPath) {
        std::error_code ec;
        std::filesystem::path rPath(remoteDirPath);
        if (!std::filesystem::exists(rPath, ec) || !std::filesystem::is_directory(rPath, ec)) {
            return false;
        }

        std::filesystem::path parentDir = rPath.parent_path();
        std::filesystem::path remCachePath = parentDir / "remotecache.vdf";
        if (!std::filesystem::exists(remCachePath, ec)) {
            if (parentDir.filename().string() != "1809700") {
                return false;
            }
        }

        std::vector<std::filesystem::path> allFiles;
        for (const auto& entry : std::filesystem::directory_iterator(rPath, ec)) {
            if (entry.is_regular_file()) {
                std::string fname = entry.path().filename().string();
                if (fname.find(".BIN") != std::string::npos) {
                    allFiles.push_back(entry.path());
                }
            }
        }

        if (allFiles.empty()) return false;

        std::ofstream vdf(remCachePath, std::ios::trunc);
        if (!vdf.is_open()) return false;

        vdf << "\"1809700\"\n{\n\t\"ChangeNumber\"\t\t\"0\"\n\t\"OSType\"\t\t\"-1\"\n";

        for (const auto& f : allFiles) {
            uintmax_t fsize = std::filesystem::file_size(f, ec);
            auto ftime = std::time(nullptr);

            std::ifstream fileStream(f, std::ios::binary);
            std::vector<uint8_t> fbuf(static_cast<size_t>(fsize));
            fileStream.read(reinterpret_cast<char*>(fbuf.data()), fsize);
            fileStream.close();

            uint8_t sha[20] = {0};
            CalculateSha1(fbuf.data(), fbuf.size(), sha);

            std::ostringstream shaHex;
            for (int i = 0; i < 20; ++i) {
                shaHex << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(sha[i]);
            }

            vdf << "\t\"" << f.filename().string() << "\"\n\t{\n";
            vdf << "\t\t\"root\"\t\t\"0\"\n";
            vdf << "\t\t\"size\"\t\t\"" << fsize << "\"\n";
            vdf << "\t\t\"localtime\"\t\t\"" << ftime << "\"\n";
            vdf << "\t\t\"time\"\t\t\"" << ftime << "\"\n";
            vdf << "\t\t\"remotetime\"\t\t\"" << ftime << "\"\n";
            vdf << "\t\t\"sha\"\t\t\"" << shaHex.str() << "\"\n";
            vdf << "\t\t\"syncstate\"\t\t\"4\"\n";
            vdf << "\t\t\"persiststate\"\t\t\"0\"\n";
            vdf << "\t\t\"platformstosync2\"\t\t\"-1\"\n";
            vdf << "\t}\n";
        }
        vdf << "}\n";
        vdf.close();
        return true;
    }

    static std::string FormatPcSaveName(int slotNumber, bool isSystem) {
        if (isSystem) return "SYSTEM.BIN";
        char buf[64];
        std::snprintf(buf, sizeof(buf), "P3PSAVE%04d.BIN", slotNumber);
        return buf;
    }

    static std::string FormatPcSlotName(int slotNumber, bool isSystem) {
        if (isSystem) return "";
        char buf[64];
        std::snprintf(buf, sizeof(buf), "P3PSAVE%04d.BINslot", slotNumber);
        return buf;
    }

private:
    static const uint8_t* GetBinSlotTemplate() {
        static const uint8_t s_template[BinSlotSizeBytes] = {
            0x53, 0x41, 0x56, 0x45, 0x30, 0x30, 0x30, 0x31, 0xfc, 0x02, 0x2a, 0xce, 0x89, 0x27, 0xb1, 0xaf,
            0xe2, 0xa8, 0x01, 0x06, 0x5d, 0x91, 0x60, 0x21, 0xe4, 0xd8, 0xf1, 0xc2, 0xcc, 0x66, 0xfb, 0x4b,
            0xb6, 0x08, 0x8b, 0x25, 0xef, 0xf7, 0x11, 0x88, 0x50, 0x65, 0x72, 0x73, 0x6f, 0x6e, 0x61, 0x33,
            0x20, 0x50, 0x4f, 0x52, 0x54, 0x41, 0x42, 0x4c, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x41, 0x56, 0x45, 0x20, 0x44, 0x41, 0x54,
            0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x93, 0xfa, 0x95, 0x74, 0x3a, 0x30, 0x34, 0x2f,
            0x30, 0x37, 0x3a, 0x35, 0x0a, 0x96, 0xbc, 0x91, 0x4f, 0x3a, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1,
            0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef,
            0xbc, 0xa1, 0x20, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef,
            0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0xef, 0xbc, 0xa1, 0x20, 0x4c, 0x76, 0x3a, 0x31,
            0x0a, 0x90, 0xab, 0x95, 0xca, 0x3a, 0x30, 0x0a, 0x8f, 0xea, 0x8f, 0x8a, 0x3a, 0x37, 0x2f, 0x32,
            0x0a, 0x83, 0x76, 0x83, 0x8c, 0x83, 0x43, 0x8e, 0x9e, 0x8a, 0xd4, 0x30, 0x3a, 0x30, 0x32, 0x0a,
            0x93, 0xef, 0x88, 0xd5, 0x93, 0x78, 0x3a, 0x32, 0x0a, 0x4c, 0x41, 0x4e, 0x47, 0x35, 0x0a, 0x83,
            0x4e, 0x83, 0x8a, 0x83, 0x41, 0x89, 0xf1, 0x90, 0x94, 0x3a, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xea, 0x07, 0x09, 0x00,
            0x13, 0x00, 0x0b, 0x00, 0x0b, 0x00, 0x26, 0x00, 0xd0, 0x42, 0x0b, 0x00
        };
        return s_template;
    }
};

} // namespace Ps4Atlus
