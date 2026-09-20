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

namespace Ps4Atlus {

class SH2Crypt {
public:
    // PC save structure:
    //   %APPDATA%\SEGA\SOULHACKERS2\Steam\<SteamID>\SaveData\
    //     SAVE01/ .. SAVE20/   -> data.dat + system.dat + steam_autocloud.vdf
    //     AUTOSAVE/            -> data.dat + system.dat + steam_autocloud.vdf
    //     SYSTEMDATA/          -> data.dat + system.dat + steam_autocloud.vdf
    //     SYSTEM2DATA/         -> data.dat + system.dat
    //
    // No encryption, no hashing. Just copy data.dat into the correct slot folder
    // and generate system.dat (UTF-16LE JSON) + steam_autocloud.vdf.

    static constexpr int MaxSlots = 20;

    struct SH2Metadata {
        int slotNumber = 1;
        bool isSystem = false;
        bool isAutoSave = false;
        bool isSystem2 = false;
        std::string protagonistName = "Ringo";
        int level = 1;
        int partySize = 1;
        std::string playtimeFormatted = "0h 00m";
        uint32_t playtimeHours = 0;
        uint32_t playtimeMinutes = 0;
        uint32_t playtimeSeconds = 0;
        std::string difficulty = "NORMAL";
        size_t fileSize = 0;
        std::string displayLabel; // Pre-built display string from system.dat detail
    };

    // Parse the system.dat (UTF-16LE JSON) to extract metadata for display
    static bool ParseSystemDat(const std::string& systemDatPath, SH2Metadata& meta) {
        std::error_code ec;
        if (!std::filesystem::exists(systemDatPath, ec)) return false;

        std::ifstream f(systemDatPath, std::ios::binary);
        if (!f.is_open()) return false;

        f.seekg(0, std::ios::end);
        size_t fsize = static_cast<size_t>(f.tellg());
        f.seekg(0, std::ios::beg);

        std::vector<uint8_t> raw(fsize);
        f.read(reinterpret_cast<char*>(raw.data()), fsize);
        f.close();

        // Convert UTF-16LE to narrow string (ASCII-safe for our fields)
        std::string text;
        for (size_t i = 0; i + 1 < raw.size(); i += 2) {
            uint16_t ch = raw[i] | (raw[i + 1] << 8);
            if (ch < 128) {
                text.push_back(static_cast<char>(ch));
            } else {
                text.push_back('?'); // Non-ASCII placeholder
            }
        }

        // Extract "detail" field content
        auto extractJsonString = [&](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\": \"";
            auto pos = text.find(search);
            if (pos == std::string::npos) {
                search = "\"" + key + "\":\""; // no space
                pos = text.find(search);
            }
            if (pos == std::string::npos) return "";
            size_t valStart = pos + search.size();
            // Find closing quote (handle escaped chars)
            std::string result;
            for (size_t i = valStart; i < text.size(); ++i) {
                if (text[i] == '\\' && i + 1 < text.size()) {
                    if (text[i + 1] == 'n') { result.push_back('\n'); i++; }
                    else if (text[i + 1] == '"') { result.push_back('"'); i++; }
                    else if (text[i + 1] == '\\') { result.push_back('\\'); i++; }
                    else { result.push_back(text[i + 1]); i++; }
                } else if (text[i] == '"') {
                    break;
                } else {
                    result.push_back(text[i]);
                }
            }
            return result;
        };

        std::string detail = extractJsonString("detail");
        std::string dirName = extractJsonString("dirName");

        // Parse dirName for slot info
        std::string lowerDir = dirName;
        std::transform(lowerDir.begin(), lowerDir.end(), lowerDir.begin(),
                       [](unsigned char c) { return (char)::tolower(c); });

        if (lowerDir.find("autosave") != std::string::npos) {
            meta.isAutoSave = true;
            meta.slotNumber = 0;
        } else if (lowerDir.find("system2data") != std::string::npos) {
            meta.isSystem2 = true;
            meta.isSystem = true;
            meta.slotNumber = 0;
        } else if (lowerDir.find("systemdata") != std::string::npos) {
            meta.isSystem = true;
            meta.slotNumber = 0;
        } else {
            // Extract slot number from SAVE01, SAVE02, etc.
            std::regex slotRe(R"((\d+))");
            std::smatch match;
            if (std::regex_search(dirName, match, slotRe)) {
                meta.slotNumber = std::stoi(match[1].str());
            }
        }

        // Parse detail lines for display metadata
        if (!detail.empty()) {
            std::istringstream ss(detail);
            std::string line;
            while (std::getline(ss, line)) {
                // "Ringo Niv. 38"
                if (line.find("Ringo") != std::string::npos) {
                    std::regex lvlRe(R"(Niv\.\s*(\d+))");
                    std::smatch m;
                    if (std::regex_search(line, m, lvlRe)) {
                        meta.level = std::stoi(m[1].str());
                    }
                    meta.protagonistName = "Ringo";
                }
                // "TEMPS DE JEU 19:8:8"
                if (line.find("TEMPS DE JEU") != std::string::npos || line.find("PLAY TIME") != std::string::npos) {
                    std::regex timeRe(R"((\d+):(\d+):(\d+))");
                    std::smatch m;
                    if (std::regex_search(line, m, timeRe)) {
                        meta.playtimeHours = static_cast<uint32_t>(std::stoi(m[1].str()));
                        meta.playtimeMinutes = static_cast<uint32_t>(std::stoi(m[2].str()));
                        meta.playtimeSeconds = static_cast<uint32_t>(std::stoi(m[3].str()));
                        char buf[32];
                        std::snprintf(buf, sizeof(buf), "%uh %02um", meta.playtimeHours, meta.playtimeMinutes);
                        meta.playtimeFormatted = buf;
                    }
                }
                // "DIFFICULT? EASY" or "DIFFICULT? NORMAL"
                if (line.find("DIFFICULT") != std::string::npos) {
                    if (line.find("EASY") != std::string::npos) meta.difficulty = "EASY";
                    else if (line.find("HARD") != std::string::npos) meta.difficulty = "HARD";
                    else if (line.find("VERY HARD") != std::string::npos) meta.difficulty = "VERY HARD";
                    else meta.difficulty = "NORMAL";
                }
                // "Membres du groupe 4"
                if (line.find("Membres du groupe") != std::string::npos || line.find("Party Members") != std::string::npos) {
                    std::regex partySizeRe(R"((\d+))");
                    std::smatch m;
                    if (std::regex_search(line, m, partySizeRe)) {
                        meta.partySize = std::stoi(m[1].str());
                    }
                }
            }
        }

        return true;
    }

    // Inspect a PS4 decrypted save file (data.dat or GAME.BIN from the PS4 dump)
    static bool InspectSaveFile(const std::string& filePath,
                                int& outSlot,
                                bool& outIsSystem,
                                bool& outIsAutoSave,
                                std::string& outDisplayName,
                                SH2Metadata* outMeta = nullptr) {
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

        // Detect system saves
        if (lowerParent.find("system2data") != std::string::npos ||
            lowerName.find("system2data") != std::string::npos) {
            outIsSystem = true;
            outIsAutoSave = false;
            outSlot = 0;
            outDisplayName = "SYSTEM2DATA";
            if (outMeta) {
                outMeta->isSystem = true;
                outMeta->isSystem2 = true;
                outMeta->slotNumber = 0;
                outMeta->protagonistName = "Système 2";
                outMeta->fileSize = fsize;
            }
            return true;
        }

        if (lowerParent.find("systemdata") != std::string::npos ||
            lowerName.find("systemdata") != std::string::npos ||
            (lowerParent.find("system") != std::string::npos && lowerParent.find("system2") == std::string::npos)) {
            outIsSystem = true;
            outIsAutoSave = false;
            outSlot = 0;
            outDisplayName = "SYSTEMDATA";
            if (outMeta) {
                outMeta->isSystem = true;
                outMeta->slotNumber = 0;
                outMeta->protagonistName = "Système";
                outMeta->fileSize = fsize;
            }
            return true;
        }

        // Detect autosave
        if (lowerParent.find("autosave") != std::string::npos ||
            lowerName.find("autosave") != std::string::npos) {
            outIsSystem = false;
            outIsAutoSave = true;
            outSlot = 0;
            outDisplayName = "AUTOSAVE";
            if (outMeta) {
                outMeta->isAutoSave = true;
                outMeta->slotNumber = 0;
                outMeta->fileSize = fsize;
            }
            // Try to parse system.dat next to data.dat
            std::filesystem::path sysDatPath = std::filesystem::path(filePath).parent_path() / "system.dat";
            if (outMeta && std::filesystem::exists(sysDatPath, ec)) {
                ParseSystemDat(sysDatPath.string(), *outMeta);
            }
            return true;
        }

        // Regular game save
        outIsSystem = false;
        outIsAutoSave = false;
        outSlot = 1;

        // Extract slot number from parent directory or filename
        std::string combined = parentDir + " " + fname;
        std::regex slotRegex(R"((\d{1,2}))");
        std::smatch match;
        if (std::regex_search(combined, match, slotRegex)) {
            int parsed = std::stoi(match[1].str());
            if (parsed >= 1 && parsed <= MaxSlots) {
                outSlot = parsed;
            }
        }

        char buf[32];
        std::snprintf(buf, sizeof(buf), "SAVE%02d", outSlot);
        outDisplayName = buf;

        if (outMeta) {
            outMeta->slotNumber = outSlot;
            outMeta->isSystem = false;
            outMeta->fileSize = fsize;

            // Try to parse system.dat next to data.dat
            std::filesystem::path sysDatPath = std::filesystem::path(filePath).parent_path() / "system.dat";
            if (std::filesystem::exists(sysDatPath, ec)) {
                ParseSystemDat(sysDatPath.string(), *outMeta);
            }
        }

        return true;
    }

    // Format the PC target folder name
    static std::string FormatPcDirName(int slot, bool isSystem, bool isSystem2, bool isAutoSave) {
        if (isSystem2) return "SYSTEM2DATA";
        if (isSystem) return "SYSTEMDATA";
        if (isAutoSave) return "AUTOSAVE";
        char buf[32];
        std::snprintf(buf, sizeof(buf), "SAVE%02d", slot);
        return buf;
    }

    // Generate system.dat content (UTF-16LE JSON) for a given slot
    static std::vector<uint8_t> GenerateSystemDat(const std::string& dirName,
                                                   const SH2Metadata& meta) {
        // Build the JSON
        std::string detail;
        if (!meta.isSystem && !meta.isSystem2) {
            // Game save or autosave: include detail block
            detail += "Membres du groupe " + std::to_string(meta.partySize) + "\\n";
            detail += "Ringo Niv. " + std::to_string(meta.level) + "\\n";
            // Additional party members could go here but we'll keep it simple
            char timeBuf[32];
            std::snprintf(timeBuf, sizeof(timeBuf), "%u:%u:%u",
                          meta.playtimeHours, meta.playtimeMinutes, meta.playtimeSeconds);
            detail += "TEMPS DE JEU ";
            detail += timeBuf;
            detail += "\\nDIFFICULT\\u00c9 " + meta.difficulty;
            detail += "\\nCarte 1001,0,0\\nDONN\\u00c9ES D'ACH\\u00c8VEMENT 0\\nSAISON 0\\n";
        }

        std::string subTitle;
        if (meta.isSystem || meta.isSystem2) {
            subTitle = "Donn\\u00e9es syst\\u00e8me";
        } else if (meta.isAutoSave) {
            subTitle = "Donn\\u00e9es sauvegard\\u00e9es (Sauvegarde auto)";
        } else {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Donn\\u00e9es sauvegard\\u00e9es %02d", meta.slotNumber);
            subTitle = buf;
        }

        // Get current time as .NET ticks (100-nanosecond intervals since 0001-01-01)
        auto now = std::chrono::system_clock::now();
        auto epoch = now.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
        // .NET epoch offset: 621355968000000000 ticks (Jan 1, 1970 in .NET ticks)
        int64_t ticks = (seconds * 10000000LL) + 621355968000000000LL;

        // userParam: use default value
        uint32_t userParam = 2147483648u; // 0x80000000
        if (!meta.isSystem && !meta.isSystem2 && meta.level > 1) {
            userParam = 2147484673u; // observed value for active saves
        }

        std::ostringstream json;
        json << "{\n";
        json << "    \"dirName\": \"" << dirName << "\",\n";
        json << "    \"title\": \"Soul Hackers 2\",\n";
        json << "    \"subTitle\": \"" << subTitle << "\",\n";
        json << "    \"detail\": \"" << detail << "\",\n";
        json << "    \"userParam\": " << userParam << ",\n";
        json << "    \"mtime\": " << ticks << "\n";
        json << "}\n";

        std::string jsonStr = json.str();

        // Convert to UTF-16LE
        std::vector<uint8_t> result;
        result.reserve(jsonStr.size() * 2);
        for (char c : jsonStr) {
            result.push_back(static_cast<uint8_t>(c));
            result.push_back(0);
        }
        return result;
    }

    // Generate steam_autocloud.vdf content
    static std::string GenerateSteamAutocloud(const std::string& steamId) {
        return "\"steam_autocloud.vdf\"\n{\n\t\"accountid\"\t\t\"" + steamId + "\"\n}\n";
    }

    // Clean DLC flags in system.dat (UTF-16LE JSON) by resetting userParam to base 2147483648 (0x80000000)
    static bool CleanDlcFlagsInSystemDat(std::vector<uint8_t>& sysData, bool& outChanged) {
        outChanged = false;
        if (sysData.size() < 24) return false;

        // Pattern for "userParam" in UTF-16LE:
        static const uint8_t pattern[] = {
            '"', 0, 'u', 0, 's', 0, 'e', 0, 'r', 0, 'P', 0, 'a', 0, 'r', 0, 'a', 0, 'm', 0, '"', 0
        };
        constexpr size_t patLen = sizeof(pattern);

        auto it = std::search(sysData.begin(), sysData.end(), pattern, pattern + patLen);
        if (it == sysData.end()) return false;

        size_t pos = std::distance(sysData.begin(), it) + patLen;
        // Skip ':' and whitespace in UTF-16LE
        while (pos + 1 < sysData.size()) {
            uint16_t ch = sysData[pos] | (sysData[pos + 1] << 8);
            if (ch == ':' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                pos += 2;
            } else {
                break;
            }
        }

        // Find end of digits
        size_t digitStart = pos;
        while (pos + 1 < sysData.size()) {
            uint16_t ch = sysData[pos] | (sysData[pos + 1] << 8);
            if (ch >= '0' && ch <= '9') {
                pos += 2;
            } else {
                break;
            }
        }
        size_t digitEnd = pos;
        if (digitEnd <= digitStart) return false;

        // Check if already 2147483648
        std::string currentVal;
        for (size_t i = digitStart; i < digitEnd; i += 2) {
            currentVal.push_back(static_cast<char>(sysData[i]));
        }

        if (currentVal != "2147483648") {
            outChanged = true;
            // Build replacement "2147483648" in UTF-16LE
            std::string newVal = "2147483648";
            std::vector<uint8_t> newBytes;
            for (char c : newVal) {
                newBytes.push_back(static_cast<uint8_t>(c));
                newBytes.push_back(0);
            }
            sysData.erase(sysData.begin() + digitStart, sysData.begin() + digitEnd);
            sysData.insert(sysData.begin() + digitStart, newBytes.begin(), newBytes.end());
        }

        return true;
    }

    // Convert (copy) a PS4 decrypted save to the PC format
    // Creates the target directory structure: SAVE##/data.dat + system.dat + steam_autocloud.vdf
    static bool ConvertSaveFile(const std::string& inputDataDatPath,
                                const std::string& outputDir,
                                int slotNumber,
                                bool isSystem,
                                bool isSystem2,
                                bool isAutoSave,
                                const std::string& steamId,
                                std::string& error,
                                bool removeDlcFlags = true,
                                bool* outDlcCleaned = nullptr) {
        std::error_code ec;
        if (outDlcCleaned) *outDlcCleaned = false;

        // Determine target directory name
        std::string dirName = FormatPcDirName(slotNumber, isSystem, isSystem2, isAutoSave);
        std::filesystem::path targetDir = std::filesystem::path(outputDir) / dirName;

        // Create target directory
        std::filesystem::create_directories(targetDir, ec);
        if (ec) {
            error = "Impossible de créer le dossier : " + targetDir.string();
            return false;
        }

        // Read source data.dat
        std::ifstream inFile(inputDataDatPath, std::ios::binary);
        if (!inFile.is_open()) {
            error = "Impossible d'ouvrir le fichier source : " + inputDataDatPath;
            return false;
        }
        inFile.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(inFile.tellg());
        inFile.seekg(0, std::ios::beg);
        std::vector<uint8_t> data(fileSize);
        inFile.read(reinterpret_cast<char*>(data.data()), fileSize);
        inFile.close();

        // Write data.dat
        std::filesystem::path dataDatPath = targetDir / "data.dat";
        bool skipWriteData = false;
        if (isSystem && !isSystem2 && std::filesystem::exists(dataDatPath, ec)) {
            // PC SYSTEMDATA contains PC keyboard/mouse/gamepad mappings (m_ConfigSystemSaveData).
            // PS4 SYSTEMDATA lacks these fields. Never overwrite an existing PC SYSTEMDATA.
            skipWriteData = true;
        }

        if (!skipWriteData) {
            std::ofstream outData(dataDatPath, std::ios::binary);
            if (!outData.is_open()) {
                error = "Impossible de créer data.dat dans : " + targetDir.string();
                return false;
            }
            outData.write(reinterpret_cast<const char*>(data.data()), data.size());
            outData.close();
        }

        // Handle system.dat: prefer copying authentic source system.dat if present
        std::filesystem::path sysDatPath = targetDir / "system.dat";
        std::filesystem::path srcSysDat = std::filesystem::path(inputDataDatPath).parent_path() / "system.dat";

        if (std::filesystem::exists(srcSysDat, ec)) {
            // Copy authentic source system.dat directly for 100% compatibility
            std::ifstream inSys(srcSysDat, std::ios::binary);
            if (inSys.is_open()) {
                inSys.seekg(0, std::ios::end);
                size_t sysSz = static_cast<size_t>(inSys.tellg());
                inSys.seekg(0, std::ios::beg);
                std::vector<uint8_t> sysBuf(sysSz);
                inSys.read(reinterpret_cast<char*>(sysBuf.data()), sysSz);
                inSys.close();

                // Clean DLC requirements if requested
                if (removeDlcFlags) {
                    bool cleaned = false;
                    CleanDlcFlagsInSystemDat(sysBuf, cleaned);
                    if (cleaned && outDlcCleaned) *outDlcCleaned = true;
                }

                std::ofstream outSys(sysDatPath, std::ios::binary);
                if (outSys.is_open()) {
                    outSys.write(reinterpret_cast<const char*>(sysBuf.data()), sysBuf.size());
                    outSys.close();
                }
            }
        } else {
            // Fallback generation only if source system.dat was not provided
            SH2Metadata meta;
            meta.slotNumber = slotNumber;
            meta.isSystem = isSystem;
            meta.isSystem2 = isSystem2;
            meta.isAutoSave = isAutoSave;
            meta.fileSize = fileSize;

            auto systemDatContent = GenerateSystemDat(dirName, meta);
            if (removeDlcFlags) {
                bool cleaned = false;
                CleanDlcFlagsInSystemDat(systemDatContent, cleaned);
                if (cleaned && outDlcCleaned) *outDlcCleaned = true;
            }

            std::ofstream outSys(sysDatPath, std::ios::binary);
            if (!outSys.is_open()) {
                error = "Impossible de créer system.dat dans : " + targetDir.string();
                return false;
            }
            outSys.write(reinterpret_cast<const char*>(systemDatContent.data()), systemDatContent.size());
            outSys.close();
        }

        // Generate steam_autocloud.vdf
        std::string vdfContent = GenerateSteamAutocloud(steamId);
        std::filesystem::path vdfPath = targetDir / "steam_autocloud.vdf";
        std::ofstream outVdf(vdfPath, std::ios::trunc);
        if (!outVdf.is_open()) {
            error = "Impossible de créer steam_autocloud.vdf dans : " + targetDir.string();
            return false;
        }
        outVdf << vdfContent;
        outVdf.close();

        return true;
    }

    // Detect Steam ID from existing save directory
    static std::string DetectSteamId(const std::string& saveDataDir) {
        std::error_code ec;
        // Check any existing steam_autocloud.vdf
        for (const auto& entry : std::filesystem::recursive_directory_iterator(saveDataDir, std::filesystem::directory_options::skip_permission_denied, ec)) {
            if (entry.is_regular_file() && entry.path().filename().string() == "steam_autocloud.vdf") {
                std::ifstream f(entry.path());
                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                f.close();
                std::regex idRe("\"accountid\"\\s+\"(\\d+)\"");
                std::smatch match;
                if (std::regex_search(content, match, idRe)) {
                    return match[1].str();
                }
            }
        }

        // Try extracting from path: ...\Steam\<SteamID>\SaveData
        std::filesystem::path p(saveDataDir);
        // Walk up looking for a numeric directory after "Steam"
        auto it = p.begin();
        bool foundSteam = false;
        for (; it != p.end(); ++it) {
            std::string part = it->string();
            std::string lower = part;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return (char)::tolower(c); });
            if (lower == "steam") {
                foundSteam = true;
            } else if (foundSteam) {
                // Check if this part is numeric
                bool allDigit = !part.empty();
                for (char c : part) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) {
                        allDigit = false;
                        break;
                    }
                }
                if (allDigit) return part;
                break;
            }
        }

        return "0"; // fallback
    }
};

} // namespace Ps4Atlus
