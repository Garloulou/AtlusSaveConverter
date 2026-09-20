#pragma once

#include "models/GameProfile.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <regex>
#include <algorithm>
#include <cstdint>
#include <shlobj.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Ps4Atlus {

struct SteamAccount {
    std::string steamId64;
    std::string steamId32;
    std::string accountName;
    std::string personaName;
    bool isActive = false;
    uint64_t timestamp = 0;
};

class SteamDetector {
public:
    static std::string GetSteamInstallPath() {
#ifdef _WIN32
        HKEY hKey;
        char buffer[MAX_PATH] = {0};
        DWORD bufferSize = sizeof(buffer);
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hKey, "SteamPath", NULL, NULL, reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                std::string p = buffer;
                std::replace(p.begin(), p.end(), '/', '\\');
                return p;
            }
            RegCloseKey(hKey);
        }
#endif
        std::error_code ec;
        if (std::filesystem::exists("C:\\Program Files (x86)\\Steam", ec)) {
            return "C:\\Program Files (x86)\\Steam";
        }
        if (std::filesystem::exists("C:\\Program Files\\Steam", ec)) {
            return "C:\\Program Files\\Steam";
        }
        return "C:\\Program Files (x86)\\Steam";
    }

    static std::vector<SteamAccount> DetectAccounts() {
        std::vector<SteamAccount> accounts;
        std::string steamDir = GetSteamInstallPath();
        std::filesystem::path steamPath(steamDir);
        std::error_code ec;

        // 1. Read loginusers.vdf
        std::filesystem::path loginUsersPath = steamPath / "config" / "loginusers.vdf";
        if (std::filesystem::exists(loginUsersPath, ec)) {
            std::ifstream file(loginUsersPath);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();

                std::regex blockRegex("\"(\\d{15,25})\"\\s*\\{([^}]+)\\}");
                std::regex accRegex("\"AccountName\"\\s*\"([^\"]+)\"");
                std::regex perRegex("\"PersonaName\"\\s*\"([^\"]+)\"");
                std::regex autoRegex("\"AutoLogin\"\\s*\"([^\"]+)\"");
                std::regex tsRegex("\"Timestamp\"\\s*\"([^\"]+)\"");

                auto words_begin = std::sregex_iterator(content.begin(), content.end(), blockRegex);
                auto words_end = std::sregex_iterator();

                for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                    std::smatch match = *i;
                    std::string id64Str = match[1].str();
                    std::string body = match[2].str();

                    uint64_t id64 = 0;
                    try { id64 = std::stoull(id64Str); } catch (...) {}
                    uint32_t id32 = static_cast<uint32_t>(id64 > 76561197960265728ULL ? id64 - 76561197960265728ULL : id64);

                    SteamAccount acc;
                    acc.steamId64 = id64Str;
                    acc.steamId32 = std::to_string(id32);

                    std::smatch subMatch;
                    if (std::regex_search(body, subMatch, perRegex)) {
                        acc.personaName = subMatch[1].str();
                    } else {
                        acc.personaName = acc.steamId32;
                    }

                    if (std::regex_search(body, subMatch, accRegex)) {
                        acc.accountName = subMatch[1].str();
                    }

                    if (std::regex_search(body, subMatch, autoRegex)) {
                        acc.isActive = (subMatch[1].str() == "1");
                    }

                    if (std::regex_search(body, subMatch, tsRegex)) {
                        try { acc.timestamp = std::stoull(subMatch[1].str()); } catch (...) {}
                    }

                    accounts.push_back(acc);
                }
            }
        }

        // 2. Scan userdata directory for any additional accounts
        std::filesystem::path userDataPath = steamPath / "userdata";
        if (std::filesystem::exists(userDataPath, ec) && std::filesystem::is_directory(userDataPath, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(userDataPath, ec)) {
                if (entry.is_directory()) {
                    std::string name = entry.path().filename().string();
                    if (!name.empty() && std::all_of(name.begin(), name.end(), ::isdigit)) {
                        bool found = false;
                        for (const auto& a : accounts) {
                            if (a.steamId32 == name) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            SteamAccount acc;
                            acc.steamId32 = name;
                            try {
                                uint64_t id32 = std::stoull(name);
                                acc.steamId64 = std::to_string(id32 + 76561197960265728ULL);
                            } catch (...) {
                                acc.steamId64 = name;
                            }
                            acc.personaName = "ID: " + name;
                            acc.isActive = false;
                            acc.timestamp = 0;
                            accounts.push_back(acc);
                        }
                    }
                }
            }
        }

        // 3. Scan %APPDATA%\SEGA\SOULHACKERS2\Steam for any SH2 specific accounts
        char appDataPath[MAX_PATH] = {0};
        SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
        std::filesystem::path sh2Steam = std::filesystem::path(appDataPath) / "SEGA" / "SOULHACKERS2" / "Steam";
        if (std::filesystem::exists(sh2Steam, ec) && std::filesystem::is_directory(sh2Steam, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(sh2Steam, ec)) {
                if (entry.is_directory()) {
                    std::string name = entry.path().filename().string();
                    if (!name.empty() && std::all_of(name.begin(), name.end(), ::isdigit)) {
                        bool found = false;
                        for (const auto& a : accounts) {
                            if (a.steamId32 == name) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            SteamAccount acc;
                            acc.steamId32 = name;
                            try {
                                uint64_t id32 = std::stoull(name);
                                acc.steamId64 = std::to_string(id32 + 76561197960265728ULL);
                            } catch (...) {
                                acc.steamId64 = name;
                            }
                            acc.personaName = "ID: " + name;
                            acc.isActive = false;
                            acc.timestamp = 0;
                            accounts.push_back(acc);
                        }
                    }
                }
            }
        }

        // 4. Scan %APPDATA%\SEGA\P5R\Steam for any P5R specific accounts
        std::filesystem::path p5rSteam = std::filesystem::path(appDataPath) / "SEGA" / "P5R" / "Steam";
        if (std::filesystem::exists(p5rSteam, ec) && std::filesystem::is_directory(p5rSteam, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(p5rSteam, ec)) {
                if (entry.is_directory()) {
                    std::string name = entry.path().filename().string();
                    if (!name.empty() && std::all_of(name.begin(), name.end(), ::isdigit)) {
                        bool found = false;
                        for (const auto& a : accounts) {
                            if (a.steamId64 == name || a.steamId32 == name) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            SteamAccount acc;
                            try {
                                uint64_t val = std::stoull(name);
                                if (val > 76561197960265728ULL) {
                                    acc.steamId64 = name;
                                    acc.steamId32 = std::to_string(val - 76561197960265728ULL);
                                } else {
                                    acc.steamId32 = name;
                                    acc.steamId64 = std::to_string(val + 76561197960265728ULL);
                                }
                            } catch (...) {
                                acc.steamId64 = name;
                                acc.steamId32 = name;
                            }
                            acc.personaName = "ID: " + acc.steamId32;
                            acc.isActive = false;
                            acc.timestamp = 0;
                            accounts.push_back(acc);
                        }
                    }
                }
            }
        }

        // 5. Scan %APPDATA%\SEGA\Steam\P5S for any P5S specific accounts
        std::filesystem::path p5sSteam = std::filesystem::path(appDataPath) / "SEGA" / "Steam" / "P5S";
        if (std::filesystem::exists(p5sSteam, ec) && std::filesystem::is_directory(p5sSteam, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(p5sSteam, ec)) {
                if (entry.is_directory()) {
                    std::string name = entry.path().filename().string();
                    if (!name.empty() && std::all_of(name.begin(), name.end(), ::isdigit)) {
                        bool found = false;
                        for (const auto& a : accounts) {
                            if (a.steamId64 == name || a.steamId32 == name) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            SteamAccount acc;
                            try {
                                uint64_t val = std::stoull(name);
                                if (val > 76561197960265728ULL) {
                                    acc.steamId64 = name;
                                    acc.steamId32 = std::to_string(val - 76561197960265728ULL);
                                } else {
                                    acc.steamId32 = name;
                                    acc.steamId64 = std::to_string(val + 76561197960265728ULL);
                                }
                            } catch (...) {
                                acc.steamId64 = name;
                                acc.steamId32 = name;
                            }
                            acc.personaName = "ID: " + acc.steamId32;
                            acc.isActive = false;
                            acc.timestamp = 0;
                            accounts.push_back(acc);
                        }
                    }
                }
            }
        }

        // 6. Scan %APPDATA%\SEGA\P5T\Steam for any P5T specific accounts
        std::filesystem::path p5tSteam = std::filesystem::path(appDataPath) / "SEGA" / "P5T" / "Steam";
        if (std::filesystem::exists(p5tSteam, ec) && std::filesystem::is_directory(p5tSteam, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(p5tSteam, ec)) {
                if (entry.is_directory()) {
                    std::string name = entry.path().filename().string();
                    if (!name.empty() && std::all_of(name.begin(), name.end(), ::isdigit)) {
                        bool found = false;
                        for (const auto& a : accounts) {
                            if (a.steamId64 == name || a.steamId32 == name) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            SteamAccount acc;
                            try {
                                uint64_t val = std::stoull(name);
                                if (val > 76561197960265728ULL) {
                                    acc.steamId64 = name;
                                    acc.steamId32 = std::to_string(val - 76561197960265728ULL);
                                } else {
                                    acc.steamId32 = name;
                                    acc.steamId64 = std::to_string(val + 76561197960265728ULL);
                                }
                            } catch (...) {
                                acc.steamId64 = name;
                                acc.steamId32 = name;
                            }
                            acc.personaName = "ID: " + acc.steamId32;
                            acc.isActive = false;
                            acc.timestamp = 0;
                            accounts.push_back(acc);
                        }
                    }
                }
            }
        }

        // 7. Sort accounts: active/autoLogin first, then timestamp descending
        std::sort(accounts.begin(), accounts.end(), [](const SteamAccount& a, const SteamAccount& b) {
            if (a.isActive != b.isActive) return a.isActive > b.isActive;
            return a.timestamp > b.timestamp;
        });

        return accounts;
    }

    static std::string GetGameSavePath(const GameProfile& profile, const SteamAccount& account) {
        char appDataPath[MAX_PATH] = {0};
        SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
        std::string steamDir = GetSteamInstallPath();
        std::error_code ec;

        if (profile.id == GameId::Persona4Golden) {
            // AppID 1113000: <SteamDir>\userdata\<SteamID32>\1113000\remote
            return steamDir + "\\userdata\\" + account.steamId32 + "\\1113000\\remote";
        } else if (profile.id == GameId::Persona3Portable) {
            // AppID 1809700: <SteamDir>\userdata\<SteamID32>\1809700\remote
            return steamDir + "\\userdata\\" + account.steamId32 + "\\1809700\\remote";
        } else if (profile.id == GameId::SoulHackers2) {
            // %APPDATA%\SEGA\SOULHACKERS2\Steam\<SteamID32>\SaveData
            return std::string(appDataPath) + "\\SEGA\\SOULHACKERS2\\Steam\\" + account.steamId32 + "\\SaveData";
        } else if (profile.id == GameId::Persona3Reload) {
            // Check if folder exists with steamId64 or steamId32 in %APPDATA%\SEGA\P3R\Steam
            std::filesystem::path p3rBase = std::filesystem::path(appDataPath) / "SEGA" / "P3R" / "Steam";
            if (std::filesystem::exists(p3rBase / account.steamId64, ec)) {
                return (p3rBase / account.steamId64).string();
            }
            if (std::filesystem::exists(p3rBase / account.steamId32, ec)) {
                return (p3rBase / account.steamId32).string();
            }
            return (p3rBase / account.steamId64).string();
        } else if (profile.id == GameId::Smt5Vengeance) {
            // Check in %APPDATA%\SEGA\SMT5V\Steam
            std::filesystem::path smtBase = std::filesystem::path(appDataPath) / "SEGA" / "SMT5V" / "Steam";
            if (std::filesystem::exists(smtBase / account.steamId32, ec)) {
                return (smtBase / account.steamId32).string();
            }
            if (std::filesystem::exists(smtBase / account.steamId64, ec)) {
                return (smtBase / account.steamId64).string();
            }
            return (smtBase / account.steamId32).string();
        } else if (profile.id == GameId::Persona5Royal) {
            // AppID 1687950: %APPDATA%\SEGA\P5R\Steam\<SteamID64>\savedata
            std::filesystem::path p5rBase = std::filesystem::path(appDataPath) / "SEGA" / "P5R" / "Steam";
            if (std::filesystem::exists(p5rBase / account.steamId64 / "savedata", ec)) {
                return (p5rBase / account.steamId64 / "savedata").string();
            }
            if (std::filesystem::exists(p5rBase / account.steamId32 / "savedata", ec)) {
                return (p5rBase / account.steamId32 / "savedata").string();
            }
            if (std::filesystem::exists(p5rBase / account.steamId64, ec)) {
                return (p5rBase / account.steamId64 / "savedata").string();
            }
            return (p5rBase / account.steamId64 / "savedata").string();
        } else if (profile.id == GameId::Persona5Strikers) {
            // AppID 1382330: %APPDATA%\SEGA\Steam\P5S\<account.steamId32>
            std::filesystem::path p5sBase = std::filesystem::path(appDataPath) / "SEGA" / "Steam" / "P5S";
            if (std::filesystem::exists(p5sBase / account.steamId32, ec)) {
                return (p5sBase / account.steamId32).string();
            }
            if (std::filesystem::exists(p5sBase / account.steamId64, ec)) {
                return (p5sBase / account.steamId64).string();
            }
            return (p5sBase / account.steamId32).string();
        } else if (profile.id == GameId::Persona5Tactica) {
            // AppID 2254740: %APPDATA%\SEGA\P5T\Steam\<account.steamId64>\Save
            std::filesystem::path p5tBase = std::filesystem::path(appDataPath) / "SEGA" / "P5T" / "Steam";
            if (std::filesystem::exists(p5tBase / account.steamId64 / "Save", ec)) {
                return (p5tBase / account.steamId64 / "Save").string();
            }
            if (std::filesystem::exists(p5tBase / account.steamId32 / "Save", ec)) {
                return (p5tBase / account.steamId32 / "Save").string();
            }
            if (std::filesystem::exists(p5tBase / account.steamId64, ec)) {
                return (p5tBase / account.steamId64 / "Save").string();
            }
            return (p5tBase / account.steamId64 / "Save").string();
        }



        return steamDir + "\\userdata\\" + account.steamId32;
    }
};

} // namespace Ps4Atlus
