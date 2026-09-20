#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <regex>
#include <chrono>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

namespace Ps4Atlus {

class P4GCrypt {
public:
    static constexpr size_t GameSaveSize = 217937; // dataXXXX.bin / GAME.BIN
    static constexpr size_t SystemSaveSize = 32768; // system.bin / CHANNELSAVE GAME.BIN
    static constexpr size_t BinSlotSize = 884;     // companion .binslot file size
    static constexpr int MaxSlots = 16;

    struct P4GMetadata {
        int slotNumber = 1;
        bool isSystem = false;
        bool isClear = false;
        int clearCount = 0;
        std::string protagonistName = "Protagoniste";
        int level = 1;
        std::string inGameDate = "04/11";
        std::string timeOfDayStr = "Après l'école";
        std::string playtimeFormatted = "00:00:00";
        std::string locationName = "Inaba";
        uint32_t money = 0;
        size_t fileSize = 0;
    };

    // Calculate MD5 hash using Windows BCrypt
    static bool ComputeMD5(const uint8_t* data, size_t length, uint8_t outHash[16]) {
#ifdef _WIN32
        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_HASH_HANDLE hHash = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, NULL, 0);
        if (!BCRYPT_SUCCESS(status)) return false;

        status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        status = BCryptHashData(hHash, const_cast<PUCHAR>(data), static_cast<ULONG>(length), 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        status = BCryptFinishHash(hHash, outHash, 16, 0);
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return BCRYPT_SUCCESS(status);
#else
        return false;
#endif
    }

    // Calculate SHA-1 hash using Windows BCrypt
    static bool ComputeSHA1(const uint8_t* data, size_t length, uint8_t outHash[20]) {
#ifdef _WIN32
        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_HASH_HANDLE hHash = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
        if (!BCRYPT_SUCCESS(status)) return false;

        status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        status = BCryptHashData(hHash, const_cast<PUCHAR>(data), static_cast<ULONG>(length), 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        status = BCryptFinishHash(hHash, outHash, 20, 0);
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return BCRYPT_SUCCESS(status);
#else
        return false;
#endif
    }

    // Helper: Compute SHA-1 hex string for a file
    static std::string ComputeSHA1Hex(const std::string& filePath) {
        std::ifstream f(filePath, std::ios::binary);
        if (!f.is_open()) return "";
        f.seekg(0, std::ios::end);
        size_t sz = static_cast<size_t>(f.tellg());
        f.seekg(0, std::ios::beg);
        std::vector<uint8_t> buf(sz);
        f.read(reinterpret_cast<char*>(buf.data()), sz);
        f.close();

        uint8_t hash[20] = {0};
        if (!ComputeSHA1(buf.data(), buf.size(), hash)) return "";

        std::ostringstream ss;
        for (int i = 0; i < 20; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    // Convert ASCII string to Fullwidth UTF-8 (P4G PC name standard)
    static std::string ToFullwidthUtf8(const std::string& asciiStr) {
        std::string result;
        for (unsigned char c : asciiStr) {
            if (c >= 0x21 && c <= 0x7E) {
                uint32_t codepoint = static_cast<uint32_t>(c) + 0xFEE0;
                result.push_back(static_cast<char>(0xEF));
                result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else {
                result.push_back(static_cast<char>(c));
            }
        }
        return result;
    }

    // Decode 16-byte Atlus character sequence (0x80, byte2) into standard ASCII/UTF-8
    static std::string DecodeAtlusName(const uint8_t* raw, size_t maxLen = 16) {
        std::string result;
        for (size_t i = 0; i + 1 < maxLen; i += 2) {
            uint8_t b1 = raw[i];
            uint8_t b2 = raw[i + 1];
            if (b1 == 0 && b2 == 0) continue; // skip leading or trailing nulls
            if (b1 == 0x80 && b2 >= 0x60) {
                result.push_back(static_cast<char>(b2 - 0x60));
            } else if (b1 == 0x20) {
                result.push_back(' ');
            }
        }
        return result;
    }

    // Convert day_idx into MM/DD string (P4G timeline base: April 1st, 2011)
    static std::string DayIndexToDate(uint16_t dayIdx) {
        // Base: 2011-04-01
        std::tm baseTm = {};
        baseTm.tm_year = 2011 - 1900;
        baseTm.tm_mon = 3; // April (0-indexed)
        baseTm.tm_mday = 1;

        std::time_t baseTime = std::mktime(&baseTm);
        if (baseTime == -1) return "04/11";

        std::time_t targetTime = baseTime + static_cast<std::time_t>(dayIdx) * 86400;
        std::tm resTm = {};
#ifdef _WIN32
        localtime_s(&resTm, &targetTime);
#else
        localtime_r(&targetTime, &resTm);
#endif
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d/%02d", resTm.tm_mon + 1, resTm.tm_mday);
        return std::string(buf);
    }

    // Map time of day ID
    static std::string TimeOfDayToString(uint8_t tod) {
        switch (tod) {
            case 1: return "Matin";
            case 2: return "Midi";
            case 3: return "Après-midi";
            case 4: return "Après l'école";
            case 5: return "Soirée";
            case 6: return "Nuit";
            default: return "Journée";
        }
    }

    // Extract metadata from a PS4 or PC save file
    static bool ExtractMetadata(const std::vector<uint8_t>& data, P4GMetadata& meta) {
        if (data.size() < 0xd00) return false;

        meta.fileSize = data.size();

        // 1. Day index at 0x04..0x06
        uint16_t dayIdx = data[4] | (data[5] << 8);
        meta.inGameDate = DayIndexToDate(dayIdx);

        // 2. Time of day at 0x06
        meta.timeOfDayStr = TimeOfDayToString(data[6]);

        // 3. Playtime in frames at 0x08..0x0c (30 fps)
        uint32_t frames = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
        uint32_t totalSec = frames / 30;
        uint32_t hours = totalSec / 3600;
        uint32_t mins = (totalSec % 3600) / 60;
        uint32_t secs = totalSec % 60;
        char timeBuf[32];
        std::snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u:%02u", hours, mins, secs);
        meta.playtimeFormatted = timeBuf;

        // 4. Protagonist name at 0x10..0x20 (Last Name) and 0x20..0x30 (First Name)
        std::string lastName = DecodeAtlusName(data.data() + 0x10, 16);
        std::string firstName = DecodeAtlusName(data.data() + 0x20, 16);
        if (!firstName.empty() || !lastName.empty()) {
            if (!firstName.empty() && !lastName.empty()) {
                meta.protagonistName = firstName + " " + lastName;
            } else if (!firstName.empty()) {
                meta.protagonistName = firstName;
            } else {
                meta.protagonistName = lastName;
            }
        }

        // 5. Money at 0x58..0x5c
        meta.money = data[0x58] | (data[0x59] << 8) | (data[0x5a] << 16) | (data[0x5b] << 24);

        // 6. Level at 0xcda
        meta.level = data[0xcda];
        if (meta.level == 0) meta.level = 1;

        // 7. Clear Data detection (0x0e or 0x4c > 0)
        uint8_t clearByte = data[0x0e];
        if (clearByte > 0 || data[0x4c] > 0) {
            meta.isClear = true;
            meta.clearCount = (clearByte > 0) ? clearByte : data[0x4c];
        }

        return true;
    }

    // Inspect a PS4 save file or PC save file
    static bool InspectSaveFile(const std::string& filePath,
                                int& outSlot,
                                bool& outIsSystem,
                                std::string& outDisplayName,
                                P4GMetadata* outMeta = nullptr) {
        std::error_code ec;
        if (!std::filesystem::exists(filePath, ec)) return false;

        uintmax_t fsize = std::filesystem::file_size(filePath, ec);
        std::string fname = std::filesystem::path(filePath).filename().string();
        std::string parentDir = std::filesystem::path(filePath).parent_path().filename().string();

        std::string lowerName = fname;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                       [](unsigned char c) { return (char)::tolower(c); });
        std::string lowerParent = parentDir;
        std::transform(lowerParent.begin(), lowerParent.end(), lowerParent.begin(),
                       [](unsigned char c) { return (char)::tolower(c); });

        // System save detection: channelsave, system.bin, systemsave
        if (lowerName == "system.bin" ||
            lowerParent.find("channelsave") != std::string::npos ||
            lowerName.find("channelsave") != std::string::npos ||
            lowerParent.find("systemsave") != std::string::npos ||
            lowerName.find("systemsave") != std::string::npos ||
            fsize == SystemSaveSize) {
            outIsSystem = true;
            outSlot = 0;
            outDisplayName = "system.bin (Données Système)";
            if (outMeta) {
                outMeta->isSystem = true;
                outMeta->slotNumber = 0;
                outMeta->protagonistName = "Données Système";
                outMeta->fileSize = fsize;
            }
            return true;
        }

        // Regular save file (dataXXXX.bin or GAME.BIN in dec_SAVELIST####)
        outIsSystem = false;
        outSlot = 1;

        // Extract slot number from parent directory or filename
        std::regex slotRegex(R"((\d{1,4}))");
        std::smatch match;
        std::string combined = parentDir + " " + fname;
        if (std::regex_search(combined, match, slotRegex)) {
            int parsed = std::stoi(match[1].str());
            if (parsed >= 1 && parsed <= MaxSlots) {
                outSlot = parsed;
            }
        }

        char buf[64];
        std::snprintf(buf, sizeof(buf), "data%04d.bin", outSlot);
        outDisplayName = buf;

        if (outMeta) {
            outMeta->slotNumber = outSlot;
            outMeta->isSystem = false;
            outMeta->fileSize = fsize;

            std::ifstream f(filePath, std::ios::binary);
            if (f.is_open()) {
                std::vector<uint8_t> data(static_cast<size_t>(fsize));
                f.read(reinterpret_cast<char*>(data.data()), fsize);
                f.close();
                ExtractMetadata(data, *outMeta);
            }
        }

        return true;
    }

    // Generate valid companion .binslot file (884 bytes)
    static std::vector<uint8_t> GenerateBinSlot(const std::vector<uint8_t>& binData,
                                                int slotNumber,
                                                bool isSystem,
                                                const P4GMetadata* optMeta = nullptr) {
        std::vector<uint8_t> slot(BinSlotSize, 0);

        // 1. Magic at 0x00: "SAVE0001"
        const char magic[] = "SAVE0001";
        std::memcpy(slot.data(), magic, 8);

        // 2. MD5 of companion .bin file at 0x18..0x28
        uint8_t binMd5[16] = {0};
        ComputeMD5(binData.data(), binData.size(), binMd5);
        std::memcpy(slot.data() + 0x18, binMd5, 16);

        // 3. Icon path at 0x2ec
        const char iconStr[] = "app0:/data/icon.png";
        std::memcpy(slot.data() + 0x2ec, iconStr, sizeof(iconStr));

        // 4. Current timestamp at 0x334 (Year, Month, Day, Hour, Minute, Second as uint16)
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm localTm = {};
#ifdef _WIN32
        localtime_s(&localTm, &tt);
#else
        localtime_r(&tt, &localTm);
#endif
        uint16_t tsFields[6] = {
            static_cast<uint16_t>(localTm.tm_year + 1900),
            static_cast<uint16_t>(localTm.tm_mon + 1),
            static_cast<uint16_t>(localTm.tm_mday),
            static_cast<uint16_t>(localTm.tm_hour),
            static_cast<uint16_t>(localTm.tm_min),
            static_cast<uint16_t>(localTm.tm_sec)
        };
        std::memcpy(slot.data() + 0x334, tsFields, sizeof(tsFields));

        if (isSystem) {
            // System save slot layout:
            // 0x2c: "SAVE DATA"
            // 0x6c: "SYSTEM DATA"
            const char saveStr[] = "SAVE DATA";
            const char sysStr[] = "SYSTEM DATA";
            std::memcpy(slot.data() + 0x2c, saveStr, sizeof(saveStr) - 1);
            std::memcpy(slot.data() + 0x6c, sysStr, sizeof(sysStr) - 1);
        } else {
            // Game save slot layout
            P4GMetadata meta;
            if (optMeta) {
                meta = *optMeta;
            } else {
                ExtractMetadata(binData, meta);
            }
            meta.slotNumber = slotNumber;

            // Fullwidth protagonist name
            std::string fwName = ToFullwidthUtf8(meta.protagonistName);

            // Title line at 0x2c (max 59 bytes): "[Fullwidth Name] Lv:[level]"
            std::string titleStr = fwName + " Lv:" + std::to_string(meta.level);
            size_t titleCopyLen = (std::min)(titleStr.size(), size_t(59));
            std::memcpy(slot.data() + 0x2c, titleStr.data(), titleCopyLen);

            // Subtitle line at 0x6c (max 123 bytes): "[Date]:[TimeOfDay]:[Location]"
            std::string tod = meta.timeOfDayStr;
            if (tod == "Après l'école") {
                tod = "Après\xe3\x80\x80l'école";
            }
            std::string loc = meta.locationName.empty() ? "Inaba" : meta.locationName;
            std::string subStr = meta.inGameDate + ":" + tod + ":" + loc;
            size_t subCopyLen = (std::min)(subStr.size(), size_t(123));
            std::memcpy(slot.data() + 0x6c, subStr.data(), subCopyLen);

            // Summary text block at 0xec (max 511 bytes)
            std::ostringstream tb;
            if (meta.isClear) {
                tb << "CLEAR DATA\n";
            }
            tb << "Date\xc2\xa0:" << meta.inGameDate << ":" << tod << "\n";
            tb << "Nom\xc2\xa0:" << fwName << " Niv.\xc2\xa0:" << meta.level << "\n";
            tb << "Difficulté\xc2\xa0:NORMAL\n";
            tb << "Lieu\xc2\xa0:" << loc << "\n";
            tb << "Temps de jeu\xc2\xa0:" << meta.playtimeFormatted << "\n";
            tb << "LANG5\n";
            tb << "Parties terminées\xc2\xa0:" << meta.clearCount << "\n";

            std::string tbStr = tb.str();
            size_t tbCopyLen = (std::min)(tbStr.size(), size_t(511));
            std::memcpy(slot.data() + 0xec, tbStr.data(), tbCopyLen);
        }

        // 5. Compute signature at 0x08..0x18: MD5( slot[0x28:] + "P4GOLDEN" )
        const char salt[] = "P4GOLDEN";
        std::vector<uint8_t> signPayload(slot.begin() + 0x28, slot.end());
        signPayload.insert(signPayload.end(), reinterpret_cast<const uint8_t*>(salt), reinterpret_cast<const uint8_t*>(salt) + 8);

        uint8_t sigMd5[16] = {0};
        ComputeMD5(signPayload.data(), signPayload.size(), sigMd5);
        std::memcpy(slot.data() + 0x08, sigMd5, 16);

        return slot;
    }

    // Format standard PC save name
    static std::string FormatPcSaveName(int slotNumber, bool isSystem) {
        if (isSystem) return "system.bin";
        char buf[32];
        std::snprintf(buf, sizeof(buf), "data%04d.bin", slotNumber);
        return buf;
    }

    // Convert PS4 decrypted save to PC format with .binslot generation
    static bool ConvertSaveFile(const std::string& inputBinPath,
                                const std::string& outputBinPath,
                                const std::string& outputBinSlotPath,
                                int targetSlot,
                                std::string& error) {
        std::error_code ec;
        if (!std::filesystem::exists(inputBinPath, ec)) {
            error = "Fichier source introuvable : " + inputBinPath;
            return false;
        }

        // Read source binary
        std::ifstream in(inputBinPath, std::ios::binary);
        if (!in.is_open()) {
            error = "Impossible d'ouvrir le fichier source : " + inputBinPath;
            return false;
        }
        in.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);
        std::vector<uint8_t> binData(fileSize);
        in.read(reinterpret_cast<char*>(binData.data()), fileSize);
        in.close();

        bool isSystem = (fileSize == SystemSaveSize);

        // For game saves (not system.bin), patch the PC header version & checksum
        // Offset 0x34: revision (0x14), offset 0x35: platform (0x01)
        // Offset 0x36: 16-bit checksum = sum(binData[0x04..0x36]) & 0xFF
        // This is strictly required by P4G PC to recognize the save as a new version.
        if (!isSystem && binData.size() >= 0x38) {
            binData[0x34] = 0x14;
            binData[0x35] = 0x01;
            uint32_t headerSum = 0;
            for (size_t i = 0x04; i < 0x36; ++i) {
                headerSum += binData[i];
            }
            binData[0x36] = static_cast<uint8_t>(headerSum & 0xFF);
            binData[0x37] = 0x00;
        }

        // Ensure target directory exists
        std::filesystem::path outBin(outputBinPath);
        std::filesystem::create_directories(outBin.parent_path(), ec);

        // Write .bin file
        std::ofstream out(outputBinPath, std::ios::binary);
        if (!out.is_open()) {
            error = "Impossible de créer le fichier cible : " + outputBinPath;
            return false;
        }
        out.write(reinterpret_cast<const char*>(binData.data()), binData.size());
        out.close();

        // Write companion .binslot file
        if (!outputBinSlotPath.empty()) {
            P4GMetadata meta;
            if (!isSystem) {
                ExtractMetadata(binData, meta);
                meta.slotNumber = targetSlot;
            }

            auto binSlot = GenerateBinSlot(binData, targetSlot, isSystem, isSystem ? nullptr : &meta);

            std::ofstream outSlot(outputBinSlotPath, std::ios::binary);
            if (!outSlot.is_open()) {
                error = "Impossible de créer le fichier .binslot : " + outputBinSlotPath;
                return false;
            }
            outSlot.write(reinterpret_cast<const char*>(binSlot.data()), binSlot.size());
            outSlot.close();
        }

        return true;
    }

    // Update Steam remotecache.vdf for P4G (App ID 1113000)
    static bool UpdateSteamRemoteCache(const std::string& destRemoteDir) {
        std::error_code ec;
        std::filesystem::path remotePath(destRemoteDir);
        std::filesystem::path vdfPath = remotePath.parent_path() / "remotecache.vdf";

        struct FileEntry {
            std::string name;
            uint64_t size;
            std::string sha1;
        };

        std::vector<FileEntry> files;
        for (const auto& entry : std::filesystem::directory_iterator(remotePath, ec)) {
            if (entry.is_regular_file()) {
                std::string fname = entry.path().filename().string();
                if (fname.size() > 4 && fname.substr(fname.size() - 4) == ".bak") continue;
                if (fname.find(".bin") == std::string::npos) continue;

                uint64_t fsz = entry.file_size(ec);
                std::string sha1 = ComputeSHA1Hex(entry.path().string());
                files.push_back({ fname, fsz, sha1 });
            }
        }

        std::sort(files.begin(), files.end(), [](const FileEntry& a, const FileEntry& b) {
            return a.name < b.name;
        });

        auto now = std::chrono::system_clock::now();
        auto epochTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        std::ostringstream ss;
        ss << "\"1113000\"\n{\n";
        for (const auto& f : files) {
            ss << "\t\"" << f.name << "\"\n";
            ss << "\t{\n";
            ss << "\t\t\"root\"\t\t\"0\"\n";
            ss << "\t\t\"size\"\t\t\"" << f.size << "\"\n";
            ss << "\t\t\"localtime\"\t\t\"" << epochTime << "\"\n";
            ss << "\t\t\"time\"\t\t\"" << epochTime << "\"\n";
            ss << "\t\t\"remotetime\"\t\t\"" << epochTime << "\"\n";
            ss << "\t\t\"sha\"\t\t\"" << f.sha1 << "\"\n";
            ss << "\t\t\"syncstate\"\t\t\"4\"\n";
            ss << "\t\t\"persiststate\"\t\t\"0\"\n";
            ss << "\t\t\"platformstosync2\"\t\t\"-1\"\n";
            ss << "\t}\n";
        }
        ss << "}\n";

        std::ofstream vdf(vdfPath, std::ios::trunc);
        if (!vdf.is_open()) return false;
        vdf << ss.str();
        vdf.close();
        return true;
    }
};

} // namespace Ps4Atlus
