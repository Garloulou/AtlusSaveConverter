#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>

#include "miniz.h"

namespace Ps4Atlus {

struct P5TMetadata {
    int slotNumber = 1;              // e.g. 1 for slot 300, 2 for 301, 100 for system, 200 for auto
    std::string dirName = "300";      // e.g. "100", "200", "201", "300", "301", etc.
    std::string title = "";           // "Données de sauvegarde 01-1", "Sauvegarde auto.", "Données système"
    std::string protagonistName = "Joker";
    int teamLevel = 1;
    float playtimeSeconds = 0.0f;
    std::string playtimeFormatted = "0h 00m";
    int difficulty = 0;               // 0: Safety/Normal, etc.
    bool isClear = false;
    int placeType = 0;                // 0: Hideout (Azito), 2: Battle/Mission
    std::string chapterQuest = "1_24052";
    uint32_t progressId = 1004100;
    bool isSystem = false;
    bool isAutoSave = false;
    bool isQuickSave = false;
};

class P5TCrypt {
public:
    // Header for .NET BinaryFormatter Lib.Save.SaveInfoData (208 bytes)
    static inline const uint8_t SAVEINFO_HEADER[208] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x0c, 0x02, 0x00, 0x00, 0x00, 0x3b, 0x5f, 0x44, 0x65, 0x76, 0x2c, 0x20, 0x56, 0x65, 0x72,
        0x73, 0x69, 0x6f, 0x6e, 0x3d, 0x30, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x30, 0x2c, 0x20, 0x43, 0x75,
        0x6c, 0x74, 0x75, 0x72, 0x65, 0x3d, 0x6e, 0x65, 0x75, 0x74, 0x72, 0x61, 0x6c, 0x2c, 0x20, 0x50,
        0x75, 0x62, 0x6c, 0x69, 0x63, 0x4b, 0x65, 0x79, 0x54, 0x6f, 0x6b, 0x65, 0x6e, 0x3d, 0x6e, 0x75,
        0x6c, 0x6c, 0x05, 0x01, 0x00, 0x00, 0x00, 0x15, 0x4c, 0x69, 0x62, 0x2e, 0x53, 0x61, 0x76, 0x65,
        0x2e, 0x53, 0x61, 0x76, 0x65, 0x49, 0x6e, 0x66, 0x6f, 0x44, 0x61, 0x74, 0x61, 0x08, 0x00, 0x00,
        0x00, 0x09, 0x6d, 0x5f, 0x44, 0x69, 0x72, 0x4e, 0x61, 0x6d, 0x65, 0x07, 0x6d, 0x5f, 0x54, 0x69,
        0x74, 0x6c, 0x65, 0x0a, 0x6d, 0x5f, 0x53, 0x75, 0x62, 0x54, 0x69, 0x74, 0x6c, 0x65, 0x08, 0x6d,
        0x5f, 0x44, 0x65, 0x74, 0x61, 0x69, 0x6c, 0x0b, 0x6d, 0x5f, 0x55, 0x73, 0x65, 0x72, 0x50, 0x61,
        0x72, 0x61, 0x6d, 0x06, 0x6d, 0x5f, 0x54, 0x69, 0x6d, 0x65, 0x08, 0x6d, 0x5f, 0x42, 0x6c, 0x6f,
        0x63, 0x6b, 0x73, 0x0c, 0x6d, 0x5f, 0x46, 0x72, 0x65, 0x65, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x73,
        0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0d, 0x10, 0x10, 0x02, 0x00, 0x00, 0x00
    };

    // Decompress GZIP buffer using raw inflate
    static bool GzipDecompress(const uint8_t* src, size_t srcLen, std::vector<uint8_t>& dest) {
        if (!src || srcLen < 18) return false;
        if (src[0] != 0x1f || src[1] != 0x8b || src[2] != 0x08) return false;

        uint8_t flg = src[3];
        size_t offset = 10;
        if (flg & 0x04) { // FEXTRA
            if (offset + 2 > srcLen) return false;
            uint16_t xlen = src[offset] | (src[offset + 1] << 8);
            offset += 2 + xlen;
        }
        if (flg & 0x08) { // FNAME
            while (offset < srcLen && src[offset] != 0) offset++;
            offset++;
        }
        if (flg & 0x10) { // FCOMMENT
            while (offset < srcLen && src[offset] != 0) offset++;
            offset++;
        }
        if (flg & 0x02) { // FHCRC
            offset += 2;
        }
        if (offset >= srcLen || srcLen - offset < 8) return false;

        size_t payloadLen = srcLen - offset - 8;

        z_stream stream;
        std::memset(&stream, 0, sizeof(stream));
        stream.next_in = const_cast<uint8_t*>(src + offset);
        stream.avail_in = static_cast<unsigned int>(payloadLen);

        // Negative window bits (-15) for raw deflate
        if (mz_inflateInit2(&stream, -MZ_DEFAULT_WINDOW_BITS) != MZ_OK) {
            return false;
        }

        dest.clear();
        uint8_t buffer[65536];
        int status = MZ_OK;
        while (status == MZ_OK) {
            stream.next_out = buffer;
            stream.avail_out = sizeof(buffer);
            status = mz_inflate(&stream, MZ_SYNC_FLUSH);
            size_t bytesProduced = sizeof(buffer) - stream.avail_out;
            if (bytesProduced > 0) {
                dest.insert(dest.end(), buffer, buffer + bytesProduced);
            }
            if (status == MZ_STREAM_END) break;
            if (status != MZ_OK && status != MZ_BUF_ERROR) {
                mz_inflateEnd(&stream);
                return false;
            }
        }
        mz_inflateEnd(&stream);
        return !dest.empty();
    }

    // Compress buffer to GZIP format using raw deflate
    static bool GzipCompress(const uint8_t* src, size_t srcLen, std::vector<uint8_t>& dest) {
        if (!src || srcLen == 0) return false;

        z_stream stream;
        std::memset(&stream, 0, sizeof(stream));
        // Negative window bits (-15) for raw deflate
        if (mz_deflateInit2(&stream, MZ_DEFAULT_COMPRESSION, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, 8, MZ_DEFAULT_STRATEGY) != MZ_OK) {
            return false;
        }

        stream.next_in = const_cast<uint8_t*>(src);
        stream.avail_in = static_cast<unsigned int>(srcLen);

        std::vector<uint8_t> rawDeflate;
        rawDeflate.resize(mz_compressBound(static_cast<mz_ulong>(srcLen)) + 128);
        stream.next_out = rawDeflate.data();
        stream.avail_out = static_cast<unsigned int>(rawDeflate.size());

        int res = mz_deflate(&stream, MZ_FINISH);
        if (res != MZ_STREAM_END) {
            mz_deflateEnd(&stream);
            return false;
        }
        rawDeflate.resize(stream.total_out);
        mz_deflateEnd(&stream);

        // Build GZIP stream: 10-byte header + raw deflate + 8-byte footer
        dest.clear();
        dest.reserve(10 + rawDeflate.size() + 8);

        // 10-byte GZIP header
        const uint8_t gzHeader[10] = { 0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a };
        dest.insert(dest.end(), gzHeader, gzHeader + 10);

        // Payload
        dest.insert(dest.end(), rawDeflate.begin(), rawDeflate.end());

        // 8-byte footer: CRC-32 (4 bytes) + uncompressed size modulo 2^32 (4 bytes)
        mz_ulong crc = mz_crc32(0, src, static_cast<mz_ulong>(srcLen));
        dest.push_back(static_cast<uint8_t>(crc & 0xFF));
        dest.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
        dest.push_back(static_cast<uint8_t>((crc >> 16) & 0xFF));
        dest.push_back(static_cast<uint8_t>((crc >> 24) & 0xFF));

        uint32_t len32 = static_cast<uint32_t>(srcLen & 0xFFFFFFFF);
        dest.push_back(static_cast<uint8_t>(len32 & 0xFF));
        dest.push_back(static_cast<uint8_t>((len32 >> 8) & 0xFF));
        dest.push_back(static_cast<uint8_t>((len32 >> 16) & 0xFF));
        dest.push_back(static_cast<uint8_t>((len32 >> 24) & 0xFF));

        return true;
    }

    // Extract metadata from decompressed or compressed Data.dat/SaveData.dat
    static bool ExtractMetadata(const std::vector<uint8_t>& rawData, P5TMetadata& meta) {
        std::vector<uint8_t> decomp;
        const std::vector<uint8_t>* pData = &rawData;

        if (rawData.size() >= 3 && rawData[0] == 0x1f && rawData[1] == 0x8b && rawData[2] == 0x08) {
            if (!GzipDecompress(rawData.data(), rawData.size(), decomp)) {
                return false;
            }
            pData = &decomp;
        }

        const auto& data = *pData;
        if (data.empty()) return false;

        // Extract Last Name (LastName or m_LastName)
        std::string lastName = "";
        std::string firstName = "";
        const char keyLn[] = "LastName";
        auto itLn = std::search(data.begin(), data.end(), keyLn, keyLn + strlen(keyLn));
        if (itLn != data.end()) {
            size_t p = std::distance(data.begin(), itLn) + strlen(keyLn);
            if (p < data.size()) {
                uint8_t tag = data[p];
                if (tag >= 0xa0 && tag <= 0xbf) {
                    size_t slen = tag - 0xa0;
                    if (p + 1 + slen <= data.size()) {
                        lastName = std::string(reinterpret_cast<const char*>(&data[p + 1]), slen);
                    }
                } else if (tag == 0xd9 && p + 2 <= data.size()) {
                    size_t slen = data[p + 1];
                    if (p + 2 + slen <= data.size()) {
                        lastName = std::string(reinterpret_cast<const char*>(&data[p + 2]), slen);
                    }
                }
            }
        }

        // Extract First Name (m_FirstName or interned index)
        const char keyFn[] = "m_Fir";
        auto itFn = std::search(data.begin(), data.end(), keyFn, keyFn + strlen(keyFn));
        if (itFn != data.end()) {
            size_t startSearch = std::distance(data.begin(), itFn);
            std::string cand = "";
            for (size_t p = startSearch + 5; p < startSearch + 35 && p < data.size(); ++p) {
                if (std::isalpha(static_cast<unsigned char>(data[p]))) {
                    cand += static_cast<char>(data[p]);
                } else if (cand.size() >= 3) {
                    if (cand != "Name" && cand != "stName" && cand != "rstName" && cand != "FirstName" && cand != "First") {
                        firstName = cand;
                        break;
                    }
                    cand.clear();
                } else {
                    cand.clear();
                }
            }
            if (firstName.empty() && cand.size() >= 3 && cand != "Name" && cand != "stName" && cand != "rstName" && cand != "FirstName") {
                firstName = cand;
            }
        }

        if (!lastName.empty() && !firstName.empty()) {
            meta.protagonistName = lastName + " " + firstName; // Japanese order as expected by P5T engine
        } else if (!lastName.empty()) {
            meta.protagonistName = lastName;
        } else if (!firstName.empty()) {
            meta.protagonistName = firstName;
        } else {
            meta.protagonistName = "Joker";
        }

        // Extract Team Level (m_TeamLV)
        const char keyLv[] = "m_TeamLV";
        auto itLv = std::search(data.begin(), data.end(), keyLv, keyLv + strlen(keyLv));
        if (itLv != data.end()) {
            size_t p = std::distance(data.begin(), itLv) + strlen(keyLv);
            if (p < data.size()) {
                uint8_t tag = data[p];
                if (tag < 0x80) {
                    meta.teamLevel = tag;
                } else if (tag == 0xcc && p + 1 < data.size()) {
                    meta.teamLevel = data[p + 1];
                } else if (tag == 0xcd && p + 2 < data.size()) {
                    meta.teamLevel = (data[p + 1] << 8) | data[p + 2];
                }
            }
        }

        // Extract Playtime (eTime)
        const char keyTime[] = "eTime";
        auto itTime = std::search(data.begin(), data.end(), keyTime, keyTime + strlen(keyTime));
        if (itTime != data.end()) {
            size_t p = std::distance(data.begin(), itTime) + strlen(keyTime);
            if (p < data.size()) {
                uint8_t tag = data[p];
                if (tag == 0xca && p + 4 < data.size()) {
                    uint32_t val = (static_cast<uint32_t>(data[p + 1]) << 24) |
                                   (static_cast<uint32_t>(data[p + 2]) << 16) |
                                   (static_cast<uint32_t>(data[p + 3]) << 8)  |
                                   (static_cast<uint32_t>(data[p + 4]));
                    float fVal = 0.0f;
                    std::memcpy(&fVal, &val, sizeof(float));
                    meta.playtimeSeconds = fVal;
                }
            }
        }

        // Extract IsClear (m_IsClear)
        const char keyClr[] = "m_IsClear";
        auto itClr = std::search(data.begin(), data.end(), keyClr, keyClr + strlen(keyClr));
        if (itClr != data.end()) {
            size_t p = std::distance(data.begin(), itClr) + strlen(keyClr);
            if (p < data.size() && data[p] == 0xc3) {
                meta.isClear = true;
            }
        }

        // Extract Difficulty (m_Difficulty)
        const char keyDiff[] = "Difficulty";
        auto itDiff = std::search(data.begin(), data.end(), keyDiff, keyDiff + strlen(keyDiff));
        if (itDiff != data.end()) {
            size_t p = std::distance(data.begin(), itDiff) + strlen(keyDiff);
            if (p < data.size()) {
                uint8_t tag = data[p];
                if (tag < 0x80) meta.difficulty = tag;
                else if (tag == 0xcc && p + 1 < data.size()) meta.difficulty = data[p + 1];
            }
        }
        if (meta.difficulty < 0 || meta.difficulty > 4) {
            meta.difficulty = 0; // Clamped to valid enum range (0: Safety, 1: Easy, 2: Normal, 3: Hard, 4: Risky)
        }

        // ChapterQuest, ProgressId, and PlaceType
        if (meta.isClear) {
            meta.placeType = 0;
            meta.chapterQuest = "2_0";
            meta.progressId = 500040;
        } else {
            meta.placeType = 2;
            if (meta.teamLevel <= 25) {
                meta.chapterQuest = "1_24025";
                meta.progressId = 1002040;
            } else if (meta.teamLevel <= 40) {
                meta.chapterQuest = "1_24051";
                meta.progressId = 1004080;
            } else {
                meta.chapterQuest = "1_24052";
                meta.progressId = 1004100;
            }
        }

        int hours = static_cast<int>(meta.playtimeSeconds / 3600.0f);
        int mins = static_cast<int>(std::fmod(meta.playtimeSeconds, 3600.0f) / 60.0f);
        char timeBuf[32];
        std::snprintf(timeBuf, sizeof(timeBuf), "%dh %02dm", hours, mins);
        meta.playtimeFormatted = timeBuf;

        return true;
    }

    // Write a .NET BinaryObjectString record
    static void WriteNetString(std::vector<uint8_t>& buf, uint32_t objectId, const std::string* str) {
        if (!str) {
            buf.push_back(0x09); // ObjectNull
            return;
        }
        buf.push_back(0x06); // BinaryObjectString
        // Object ID (4 bytes little-endian)
        buf.push_back(static_cast<uint8_t>(objectId & 0xFF));
        buf.push_back(static_cast<uint8_t>((objectId >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>((objectId >> 16) & 0xFF));
        buf.push_back(static_cast<uint8_t>((objectId >> 24) & 0xFF));

        // 7-bit encoded length
        size_t len = str->size();
        while (len >= 128) {
            buf.push_back(static_cast<uint8_t>((len & 0x7F) | 0x80));
            len >>= 7;
        }
        buf.push_back(static_cast<uint8_t>(len & 0x7F));

        // String bytes
        buf.insert(buf.end(), str->begin(), str->end());
    }

    // Serialize SaveInfoData and compress with GZIP
    static bool SerializeSaveInfoData(const P5TMetadata& meta, std::vector<uint8_t>& outGzip) {
        std::vector<uint8_t> plain;
        plain.insert(plain.end(), SAVEINFO_HEADER, SAVEINFO_HEADER + sizeof(SAVEINFO_HEADER));

        // String 3: m_DirName
        WriteNetString(plain, 3, &meta.dirName);

        // String 4: m_Title
        std::string titleStr;
        if (meta.isSystem) {
            titleStr = "Données système";
        } else if (meta.isAutoSave) {
            titleStr = "Sauvegarde auto.";
        } else {
            char sBuf[64];
            int sNum = meta.slotNumber;
            if (sNum >= 300) sNum -= 299; // 300 -> 1, 301 -> 2
            std::snprintf(sBuf, sizeof(sBuf), "Données de sauvegarde %02d-%d", sNum, sNum);
            titleStr = sBuf;
        }
        WriteNetString(plain, 4, &titleStr);

        // String 5: m_SubTitle (empty string)
        std::string emptySub = "";
        WriteNetString(plain, 5, &emptySub);

        // String 6: m_Detail
        if (meta.isSystem) {
            // Authentic PC format: MemberReference (0x09) pointing to Object 5 (empty string "")
            plain.push_back(0x09);
            plain.push_back(0x05);
            plain.push_back(0x00);
            plain.push_back(0x00);
            plain.push_back(0x00);
        } else {
            int placeType = meta.placeType;
            if (meta.isAutoSave) placeType = 2;
            std::string chap = meta.chapterQuest.empty() ? "1_24052" : meta.chapterQuest;
            uint32_t prog = meta.progressId > 0 ? meta.progressId : 1004100;
            if (meta.isClear) {
                placeType = 0;
                chap = "2_0";
                prog = 500040;
            }
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << placeType << "," << meta.protagonistName << "," << meta.teamLevel << ","
                << chap << "," << prog << ","
                << meta.playtimeSeconds << "," << (meta.isClear ? "True" : "False")
                << ",0," << meta.difficulty << ",fr";
            std::string detailStr = oss.str();
            WriteNetString(plain, 6, &detailStr);
        }

        // Primitive field: m_UserParam (uint32)
        uint32_t userParam = 300;
        try {
            userParam = static_cast<uint32_t>(std::stoul(meta.dirName));
        } catch (...) {}
        plain.push_back(static_cast<uint8_t>(userParam & 0xFF));
        plain.push_back(static_cast<uint8_t>((userParam >> 8) & 0xFF));
        plain.push_back(static_cast<uint8_t>((userParam >> 16) & 0xFF));
        plain.push_back(static_cast<uint8_t>((userParam >> 24) & 0xFF));

        // Primitive field: m_Time (uint64 ticks)
        auto now = std::chrono::system_clock::now();
        uint64_t unixSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        uint64_t ticks = (62135596800ULL + unixSec) * 10000000ULL;
        uint64_t timeVal = (2ULL << 62) | (ticks & 0x3FFFFFFFFFFFFFFFULL); // DateTimeKind.Local (2)
        for (int i = 0; i < 8; ++i) {
            plain.push_back(static_cast<uint8_t>((timeVal >> (i * 8)) & 0xFF));
        }

        // Primitive field: m_Blocks (int32 = 0)
        for (int i = 0; i < 4; ++i) plain.push_back(0x00);

        // Primitive field: m_FreeBlocks (int32 = 0)
        for (int i = 0; i < 4; ++i) plain.push_back(0x00);

        // 8 bytes padding / continuation
        for (int i = 0; i < 8; ++i) plain.push_back(0x00);

        // End byte: MessageEnd (0x0b)
        plain.push_back(0x0b);

        // GZIP compress
        return GzipCompress(plain.data(), plain.size(), outGzip);
    }

    // Inspect a PS4 or PC P5T save file / folder
    static bool InspectSaveFile(const std::string& path, P5TMetadata& outMeta, std::string& outDisplayName) {
        std::filesystem::path p(path);
        std::string filename = p.filename().string();
        std::string parentName = p.parent_path().filename().string();

        // Determine target slot dir name (e.g. 100, 200, 300...)
        std::string targetDir = "300";
        auto extractSlotDir = [](const std::string& s) -> std::string {
            // Check for dec_XXX_CUSA*
            if (s.rfind("dec_", 0) == 0) {
                size_t nextUnderscore = s.find('_', 4);
                if (nextUnderscore != std::string::npos) {
                    return s.substr(4, nextUnderscore - 4);
                }
            }
            // Check numeric 3 digits
            if (s.size() == 3 && std::all_of(s.begin(), s.end(), ::isdigit)) {
                return s;
            }
            return "";
        };

        std::string dirCand = extractSlotDir(filename);
        if (dirCand.empty()) dirCand = extractSlotDir(parentName);
        if (!dirCand.empty()) targetDir = dirCand;

        outMeta.dirName = targetDir;
        try {
            int val = std::stoi(targetDir);
            outMeta.slotNumber = val;
            if (val == 100) {
                outMeta.isSystem = true;
            } else if (val == 200 || val == 201) {
                outMeta.isAutoSave = true;
            } else if (val == 400) {
                outMeta.isQuickSave = true;
            }
        } catch (...) {
            outMeta.slotNumber = 300;
        }

        // Locate Data.dat / SaveData.dat
        std::filesystem::path dataFile = p;
        if (std::filesystem::is_directory(p)) {
            if (std::filesystem::exists(p / "Data.dat")) dataFile = p / "Data.dat";
            else if (std::filesystem::exists(p / "SaveData.dat")) dataFile = p / "SaveData.dat";
        }

        std::vector<uint8_t> raw;
        if (std::filesystem::exists(dataFile) && std::filesystem::is_regular_file(dataFile)) {
            std::ifstream f(dataFile, std::ios::binary);
            if (f.is_open()) {
                raw.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
                f.close();
                ExtractMetadata(raw, outMeta);
            }
        }

        if (outMeta.isSystem) {
            outMeta.title = "Données système";
            outDisplayName = "Données Système (100)";
        } else if (outMeta.isAutoSave) {
            outMeta.title = "Sauvegarde auto.";
            outDisplayName = "Sauvegarde Auto (" + outMeta.dirName + ") - " + outMeta.protagonistName + " (Niv. " + std::to_string(outMeta.teamLevel) + ")";
        } else {
            int slotIdx = outMeta.slotNumber >= 300 ? (outMeta.slotNumber - 299) : outMeta.slotNumber;
            char sBuf[64];
            std::snprintf(sBuf, sizeof(sBuf), "Données de sauvegarde %02d-%d", slotIdx, slotIdx);
            outMeta.title = sBuf;
            outDisplayName = "Emplacement " + (slotIdx < 10 ? ("0" + std::to_string(slotIdx)) : std::to_string(slotIdx)) +
                             " (" + outMeta.dirName + ") - " + outMeta.protagonistName + " (Niv. " + std::to_string(outMeta.teamLevel) + ")";
        }

        return true;
    }

    // Convert PS4 save directory or Data.dat to PC Steam format
    static bool ConvertSaveFile(const std::string& srcPath, const std::string& destPcSaveDir, int targetSlotOverride, std::string& outErr) {
        std::error_code ec;
        std::filesystem::path src(srcPath);

        std::filesystem::path dataSrcPath = src;
        if (std::filesystem::is_directory(src, ec)) {
            if (std::filesystem::exists(src / "Data.dat", ec)) {
                dataSrcPath = src / "Data.dat";
            } else if (std::filesystem::exists(src / "SaveData.dat", ec)) {
                dataSrcPath = src / "SaveData.dat";
            } else {
                outErr = "Fichier Data.dat ou SaveData.dat introuvable dans " + src.string();
                return false;
            }
        }

        if (!std::filesystem::exists(dataSrcPath, ec)) {
            outErr = "Fichier source introuvable : " + dataSrcPath.string();
            return false;
        }

        // Extract metadata
        P5TMetadata meta;
        std::string dispName;
        InspectSaveFile(srcPath, meta, dispName);

        // Slots 201 (DLC auto-save) and 400 (suspend save on console) are not valid PC slots
        if (meta.slotNumber == 400 || meta.dirName == "400" || meta.slotNumber == 201 || meta.dirName == "201") {
            outErr = "Le slot " + meta.dirName + " n'est pas pris en charge sur PC.";
            return false;
        }

        if (targetSlotOverride > 0 && !meta.isSystem && !meta.isAutoSave) {
            meta.slotNumber = targetSlotOverride;
            meta.dirName = std::to_string(targetSlotOverride);
            int sNum = meta.slotNumber >= 300 ? (meta.slotNumber - 299) : meta.slotNumber;
            char sBuf[64];
            std::snprintf(sBuf, sizeof(sBuf), "Données de sauvegarde %02d-%d", sNum, sNum);
            meta.title = sBuf;
        }

        // Read source Data.dat
        std::ifstream fIn(dataSrcPath, std::ios::binary);
        if (!fIn.is_open()) {
            outErr = "Impossible d'ouvrir le fichier source : " + dataSrcPath.string();
            return false;
        }
        std::vector<uint8_t> dataContent((std::istreambuf_iterator<char>(fIn)), std::istreambuf_iterator<char>());
        fIn.close();

        // Target directory on PC: <destPcSaveDir>/<meta.dirName>/
        std::filesystem::path targetDir = std::filesystem::path(destPcSaveDir) / meta.dirName;
        std::filesystem::create_directories(targetDir, ec);

        // Re-compress SaveData cleanly to eliminate any trailing padding/junk from PS4 dump
        std::vector<uint8_t> uncompressed;
        if (GzipDecompress(dataContent.data(), dataContent.size(), uncompressed)) {
            std::vector<uint8_t> cleanData;
            if (GzipCompress(uncompressed.data(), uncompressed.size(), cleanData)) {
                dataContent = std::move(cleanData);
            }
        }

        // Destination SaveData.dat
        std::filesystem::path destSaveData = targetDir / "SaveData.dat";
        std::ofstream fOutData(destSaveData, std::ios::binary);
        if (!fOutData.is_open()) {
            outErr = "Impossible d'écrire SaveData.dat dans " + targetDir.string();
            return false;
        }
        fOutData.write(reinterpret_cast<const char*>(dataContent.data()), dataContent.size());
        fOutData.close();

        // Destination SaveInfo.dat
        std::vector<uint8_t> gzipInfo;
        if (!SerializeSaveInfoData(meta, gzipInfo)) {
            outErr = "Échec de la sérialisation de SaveInfo.dat";
            return false;
        }

        std::filesystem::path destSaveInfo = targetDir / "SaveInfo.dat";
        std::ofstream fOutInfo(destSaveInfo, std::ios::binary);
        if (!fOutInfo.is_open()) {
            outErr = "Impossible d'écrire SaveInfo.dat dans " + targetDir.string();
            return false;
        }
        fOutInfo.write(reinterpret_cast<const char*>(gzipInfo.data()), gzipInfo.size());
        fOutInfo.close();

        return true;
    }
};

} // namespace Ps4Atlus
