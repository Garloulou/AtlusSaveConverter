#include "ui/MainWindow.hpp"
#include "ui/Theme.hpp"
#include "ui/FileDialog.hpp"
#include "core/P3RCrypt.hpp"
#include "core/SMT5VCrypt.hpp"
#include "core/P3PCrypt.hpp"
#include "core/SH2Crypt.hpp"
#include "core/P4GCrypt.hpp"
#include "core/P5RCrypt.hpp"
#include "core/P5SCrypt.hpp"
#include "core/P5TCrypt.hpp"

#include <imgui.h>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <shlobj.h>

namespace fs = std::filesystem;

namespace Ps4Atlus {

static std::string GetCurrentTimestamp() {
    auto now = std::time(nullptr);
    struct tm tmNow;
    localtime_s(&tmNow, &now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmNow);
    return std::string(buf);
}

MainWindow::MainWindow() {
    m_games = GetDefaultProfiles();
    m_selectedGameIndex = 0; // Persona 3 Reload by default

    m_sourcePath[0] = '\0';
    m_destPath[0] = '\0';
    m_detectedSaves.clear();

    ThemeManager::ApplyGameTheme(m_games[m_selectedGameIndex]);

    RefreshSteamAccounts();
    AutoDetectPcPath();

    AddLog("Ps4AtlusSaveExport v1.8.0 initialisé.", LogLevel::Success);
    AddLog("Jeux disponibles : P3R, SMT5V, P3P, SH2, P4G, P5R, P5S, Persona 5 Tactica (P5T).", LogLevel::Info);
    AddLog("Moteurs cryptographiques natifs prêts (AES-256-ECB/CBC, SHA-1, MD5, P3R, SH2, P5S LCG, P5T GZIP/MsgPack).", LogLevel::Success);
    AddLog("Sélectionnez votre jeu et vos sauvegardes sources pour commencer.", LogLevel::Info);
}



void MainWindow::AddLog(const std::string& message, LogLevel level) {
    m_logs.push_back({ GetCurrentTimestamp(), message, level });
    if (m_logs.size() > 300) {
        m_logs.erase(m_logs.begin());
    }
}

void MainWindow::SetSelectedGame(int index) {
    if (index < 0 || index >= static_cast<int>(m_games.size())) return;
    if (!m_games[index].isAvailable) return;
    if (index == m_selectedGameIndex) return;

    m_selectedGameIndex = index;
    ThemeManager::ApplyGameTheme(m_games[m_selectedGameIndex]);

    AddLog("Jeu actif changé : " + m_games[m_selectedGameIndex].name, LogLevel::Info);

    // Auto-detect PC destination for the new game
    AutoDetectPcPath();

    // Re-scan if source path is already set
    if (strlen(m_sourcePath) > 0) {
        ScanSourceSaves();
    }
}

void MainWindow::ScanSourceSaves() {
    m_detectedSaves.clear();
    std::string pathStr(m_sourcePath);

    if (pathStr.empty()) {
        return;
    }

    const auto& currentProfile = m_games[m_selectedGameIndex];
    std::error_code ec;

    if (fs::is_regular_file(pathStr, ec)) {
        // Single file selected
        if (currentProfile.id == GameId::Smt5Vengeance) {
            int slot = 1;
            bool isSystem = false;
            std::string displayName;
            SMT5VCrypt::Smt5Metadata meta;
            SMT5VCrypt::InspectSaveFile(pathStr, slot, isSystem, displayName, &meta);
            std::string targetName = SMT5VCrypt::FormatPcSaveName(slot, isSystem);

            std::string extra = isSystem ? "Paramètres Système" :
                                (meta.protagonistName + " (Niv. " + std::to_string(meta.level) + ") - " + meta.playtimeFormatted);

            uintmax_t size = fs::file_size(pathStr, ec);
            m_detectedSaves.push_back({
                displayName,
                pathStr,
                size,
                slot,
                slot,
                false,
                isSystem,
                extra,
                targetName,
                false,
                false,
                ""
            });
            AddLog("Fichier SMT5V détecté : " + displayName + " -> " + targetName, LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona3Portable) {
            int slot = 1;
            bool isSystem = false;
            std::string displayName;
            P3PCrypt::P3PMetadata meta;
            P3PCrypt::InspectSaveFile(pathStr, slot, isSystem, displayName, &meta);
            std::string targetName = P3PCrypt::FormatPcSaveName(slot, isSystem);

            std::string extra = isSystem ? "Paramètres Système" :
                                (!meta.protagonistName.empty() ?
                                    (meta.protagonistName + " (Niv. " + std::to_string(meta.level) + ") - " + meta.genderStr +
                                     (meta.isClear ? " [Terminé] - " : (" [" + meta.inGameDate + "] - ")) +
                                     meta.playtimeFormatted) :
                                    ("Emplacement " + std::to_string(slot)));

            uintmax_t size = fs::file_size(pathStr, ec);
            m_detectedSaves.push_back({
                displayName,
                pathStr,
                size,
                slot,
                slot,
                false,
                isSystem,
                extra,
                targetName,
                false,
                false,
                ""
            });
            AddLog("Fichier P3P détecté : " + displayName + " -> " + targetName, LogLevel::Info);
        } else if (currentProfile.id == GameId::SoulHackers2) {
            int slot = 1;
            bool isSystem = false;
            bool isAutoSave = false;
            std::string displayName;
            SH2Crypt::SH2Metadata meta;
            SH2Crypt::InspectSaveFile(pathStr, slot, isSystem, isAutoSave, displayName, &meta);
            std::string targetName = SH2Crypt::FormatPcDirName(slot, isSystem, meta.isSystem2, isAutoSave);

            std::string extra = (isSystem || meta.isSystem2) ? "Paramètres Système" :
                                (meta.level > 1 ?
                                    ("Ringo (Niv. " + std::to_string(meta.level) + ") - " + meta.difficulty + " - " + meta.playtimeFormatted) :
                                    (isAutoSave ? "Sauvegarde Automatique" : ("Emplacement " + std::to_string(slot))));

            uintmax_t size = fs::file_size(pathStr, ec);
            m_detectedSaves.push_back({
                displayName,
                pathStr,
                size,
                slot,
                slot,
                false,
                isSystem,
                extra,
                targetName,
                false,
                false,
                ""
            });
            AddLog("Fichier SH2 détecté : " + displayName + " -> " + targetName, LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona4Golden) {
            int slot = 1;
            bool isSystem = false;
            std::string displayName;
            P4GCrypt::P4GMetadata meta;
            P4GCrypt::InspectSaveFile(pathStr, slot, isSystem, displayName, &meta);
            std::string targetName = P4GCrypt::FormatPcSaveName(slot, isSystem);

            std::string extra = isSystem ? "Données Système (system.bin)" :
                                (!meta.protagonistName.empty() ?
                                    (meta.protagonistName + " (Niv. " + std::to_string(meta.level) + ")" +
                                     (meta.isClear ? " [Terminé] - " : (" [" + meta.inGameDate + "] - ")) +
                                     meta.playtimeFormatted) :
                                    ("Emplacement " + std::to_string(slot)));

            uintmax_t size = fs::file_size(pathStr, ec);
            m_detectedSaves.push_back({
                displayName,
                pathStr,
                size,
                slot,
                slot,
                false,
                isSystem,
                extra,
                targetName,
                false,
                false,
                ""
            });
            AddLog("Fichier P4G détecté : " + displayName + " -> " + targetName, LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona5Royal) {
            int slot = 1;
            bool isSystem = false;
            std::string displayName;
            P5RCrypt::P5RMetadata meta;
            P5RCrypt::InspectSaveFile(pathStr, slot, isSystem, displayName, &meta);
            std::string targetName = P5RCrypt::FormatPcSavePath(slot, isSystem);

            std::string extra = isSystem ? "Données Système (SYSTEM.DAT)" :
                                (!meta.protagonistName.empty() ?
                                    (meta.protagonistName + " (Niv. " + std::to_string(meta.level) + ") - " + meta.playtimeFormatted) :
                                    ("Emplacement " + std::to_string(slot)));

            uintmax_t size = fs::file_size(pathStr, ec);
            m_detectedSaves.push_back({
                displayName,
                pathStr,
                size,
                slot,
                slot,
                false,
                isSystem,
                extra,
                targetName,
                false,
                false,
                ""
            });
            AddLog("Fichier P5R détecté : " + displayName + " -> " + targetName, LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona5Strikers) {
            uint64_t steamId = 0;
            if (m_selectedSteamAccountIndex >= 0 && m_selectedSteamAccountIndex < static_cast<int>(m_steamAccounts.size())) {
                try { steamId = std::stoull(m_steamAccounts[m_selectedSteamAccountIndex].steamId64); } catch (...) {}
            }

            P5SMetadata meta;
            std::string displayName;
            P5SCrypt::InspectSaveFile(pathStr, steamId, meta, displayName);
            std::string targetName = "SAVEDATA.BIN";

            std::string extra;
            int filledCount = 0;
            std::string sampleHero;
            for (const auto& s : meta.slots) {
                if (s.hasData) {
                    filledCount++;
                    if (sampleHero.empty() && !s.fullName.empty()) {
                        sampleHero = s.fullName + " (Niv. " + std::to_string(s.level) + ")";
                    }
                }
            }
            if (filledCount > 0) {
                extra = std::to_string(filledCount) + " slot(s) actif(s)";
                if (!sampleHero.empty()) extra += " - " + sampleHero;
            } else {
                extra = "Conteneur 10 emplacements";
            }

            uintmax_t size = fs::file_size(pathStr, ec);
            m_detectedSaves.push_back({
                displayName,
                pathStr,
                size,
                1,
                1,
                false,
                false,
                extra,
                targetName,
                false,
                false,
                ""
            });
            AddLog("Fichier P5S détecté : " + displayName + " -> " + targetName, LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona5Tactica) {
            P5TMetadata meta;
            std::string displayName;
            P5TCrypt::InspectSaveFile(pathStr, meta, displayName);
            std::string targetName = meta.dirName + "/SaveData.dat";

            std::string extra = meta.isSystem ? "Données Système (100)" :
                                (meta.isAutoSave ? ("Sauvegarde Auto - " + meta.protagonistName + " (Niv. " + std::to_string(meta.teamLevel) + ") - " + meta.playtimeFormatted) :
                                 (meta.protagonistName + " (Niv. " + std::to_string(meta.teamLevel) + ") - " + meta.playtimeFormatted));

            uintmax_t size = fs::file_size(pathStr, ec);
            m_detectedSaves.push_back({
                displayName,
                pathStr,
                size,
                meta.slotNumber,
                meta.slotNumber,
                false,
                meta.isSystem,
                extra,
                targetName,
                false,
                false,
                ""
            });
            AddLog("Fichier P5T détecté : " + displayName + " -> " + targetName, LogLevel::Info);
        } else {
            // P3R
            int slot = 1;


            bool isAigis = false;
            bool isSystem = false;
            std::string displayName;
            P3RCrypt::InspectSaveFile(pathStr, slot, isAigis, isSystem, displayName);
            std::string targetName = P3RCrypt::FormatPcSaveName(slot, isAigis, isSystem);

            uintmax_t size = fs::file_size(pathStr, ec);
            m_detectedSaves.push_back({
                displayName,
                pathStr,
                size,
                slot,
                slot,
                isAigis,
                isSystem,
                isAigis ? "Épisode Aigis" : (isSystem ? "Système" : "Épisode Normal"),
                targetName,
                false,
                false,
                ""
            });
            AddLog("Fichier P3R détecté : " + displayName + " -> " + targetName, LogLevel::Info);
        }
    } else if (fs::is_directory(pathStr, ec)) {
        // Directory selected: recursively scan for all relevant save files
        for (const auto& entry : fs::recursive_directory_iterator(pathStr, fs::directory_options::skip_permission_denied, ec)) {
            if (entry.is_regular_file()) {
                std::string fname = entry.path().filename().string();
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)::tolower(c); });

                std::string lowerName = fname;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return (char)::tolower(c); });

                std::string parentDir = entry.path().parent_path().filename().string();
                std::string lowerParent = parentDir;
                std::transform(lowerParent.begin(), lowerParent.end(), lowerParent.begin(), [](unsigned char c) { return (char)::tolower(c); });

                if (currentProfile.id == GameId::Smt5Vengeance) {
                    // SMT5V: matches Megaten5.ps4.sav, GameSave*, SysSave*, ue4savegame*, or *.sav
                    bool isSmtFile = (lowerName.find("megaten5") != std::string::npos ||
                                      lowerName.find("gamesave") != std::string::npos ||
                                      lowerName.find("syssave") != std::string::npos ||
                                      lowerParent.find("gamesave") != std::string::npos ||
                                      lowerParent.find("syssave") != std::string::npos ||
                                      lowerName.find("ue4savegame") != std::string::npos ||
                                      ext == ".sav");

                    if (isSmtFile) {
                        int slot = 1;
                        bool isSystem = false;
                        std::string displayName;
                        SMT5VCrypt::Smt5Metadata meta;
                        SMT5VCrypt::InspectSaveFile(entry.path().string(), slot, isSystem, displayName, &meta);
                        std::string targetName = SMT5VCrypt::FormatPcSaveName(slot, isSystem);

                        std::string extra = isSystem ? "Paramètres Système" :
                                            (!meta.protagonistName.empty() ?
                                                (meta.protagonistName + " (Niv. " + std::to_string(meta.level) + ") - " + meta.playtimeFormatted) :
                                                ("Slot " + std::to_string(slot)));

                        m_detectedSaves.push_back({
                            displayName,
                            entry.path().string(),
                            entry.file_size(ec),
                            slot,
                            slot,
                            false,
                            isSystem,
                            extra,
                            targetName,
                            false,
                            false,
                            ""
                        });
                    }
                } else if (currentProfile.id == GameId::Persona3Portable) {
                    // P3P: matches p3psave*, data*, savedata*, savelist*, systemsave*, cusa3387*, cusa3388*, .bin, .dat
                    bool isP3pFile = (lowerName.find("p3psave") != std::string::npos ||
                                      lowerName.find("data") != std::string::npos ||
                                      lowerName.find("savedata") != std::string::npos ||
                                      lowerParent.find("savedata") != std::string::npos ||
                                      lowerParent.find("savelist") != std::string::npos ||
                                      lowerParent.find("systemsave") != std::string::npos ||
                                      lowerName.find("cusa3387") != std::string::npos ||
                                      lowerParent.find("cusa3387") != std::string::npos ||
                                      lowerName.find("cusa3388") != std::string::npos ||
                                      lowerParent.find("cusa3388") != std::string::npos ||
                                      ext == ".bin" || ext == ".dat");

                    if (isP3pFile) {
                        int slot = 1;
                        bool isSystem = false;
                        std::string displayName;
                        P3PCrypt::P3PMetadata meta;
                        P3PCrypt::InspectSaveFile(entry.path().string(), slot, isSystem, displayName, &meta);
                        std::string targetName = P3PCrypt::FormatPcSaveName(slot, isSystem);

                        std::string extra = isSystem ? "Paramètres Système" :
                                            (!meta.protagonistName.empty() ?
                                                (meta.protagonistName + " (Niv. " + std::to_string(meta.level) + ") - " + meta.genderStr +
                                                 (meta.isClear ? " [Terminé] - " : (" [" + meta.inGameDate + "] - ")) +
                                                 meta.playtimeFormatted) :
                                                ("Emplacement " + std::to_string(slot)));

                        m_detectedSaves.push_back({
                            displayName,
                            entry.path().string(),
                            entry.file_size(ec),
                            slot,
                            slot,
                            false,
                            isSystem,
                            extra,
                            targetName,
                            false,
                            false,
                            ""
                        });
                    }
                } else if (currentProfile.id == GameId::SoulHackers2) {
                    // SH2: matches data.dat files in SAVE##, AUTOSAVE, SYSTEMDATA, SYSTEM2DATA folders
                    // Also matches cusa2732* folders from PS4 dumps
                    bool isSh2File = (lowerName == "data.dat" ||
                                      lowerParent.find("save") != std::string::npos ||
                                      lowerParent.find("autosave") != std::string::npos ||
                                      lowerParent.find("systemdata") != std::string::npos ||
                                      lowerParent.find("system2data") != std::string::npos ||
                                      lowerParent.find("cusa2732") != std::string::npos ||
                                      lowerName.find("cusa2732") != std::string::npos);

                    // Skip system.dat and steam_autocloud.vdf
                    if (lowerName == "system.dat" || lowerName == "steam_autocloud.vdf") {
                        isSh2File = false;
                    }

                    if (isSh2File) {
                        int slot = 1;
                        bool isSystem = false;
                        bool isAutoSave = false;
                        std::string displayName;
                        SH2Crypt::SH2Metadata meta;
                        SH2Crypt::InspectSaveFile(entry.path().string(), slot, isSystem, isAutoSave, displayName, &meta);
                        std::string targetName = SH2Crypt::FormatPcDirName(slot, isSystem, meta.isSystem2, isAutoSave);

                        std::string extra = (isSystem || meta.isSystem2) ? "Paramètres Système" :
                                            (meta.level > 1 ?
                                                ("Ringo (Niv. " + std::to_string(meta.level) + ") - " + meta.difficulty + " - " + meta.playtimeFormatted) :
                                                (isAutoSave ? "Sauvegarde Automatique" : ("Emplacement " + std::to_string(slot))));

                        m_detectedSaves.push_back({
                            displayName,
                            entry.path().string(),
                            entry.file_size(ec),
                            slot,
                            slot,
                            false,
                            isSystem,
                            extra,
                            targetName,
                            false,
                            false,
                            ""
                        });
                    }
                } else if (currentProfile.id == GameId::Persona4Golden) {
                    // P4G: matches data*.bin, system.bin, dec_savelist*, dec_channelsave*, cusa33874*, cusa3388*
                    // Excludes .binslot files and 188-byte systemsave
                    bool isP4gFile = (lowerName.find("data") != std::string::npos ||
                                      lowerName.find("system.bin") != std::string::npos ||
                                      lowerParent.find("savelist") != std::string::npos ||
                                      lowerParent.find("channelsave") != std::string::npos ||
                                      lowerParent.find("cusa33874") != std::string::npos ||
                                      lowerName.find("cusa33874") != std::string::npos ||
                                      lowerParent.find("cusa3388") != std::string::npos ||
                                      lowerName.find("cusa3388") != std::string::npos ||
                                      ext == ".bin");

                    if (ext == ".binslot" || lowerParent.find("systemsave") != std::string::npos || lowerName.find("systemsave") != std::string::npos) {
                        isP4gFile = false;
                    }

                    if (isP4gFile) {
                        int slot = 1;
                        bool isSystem = false;
                        std::string displayName;
                        P4GCrypt::P4GMetadata meta;
                        P4GCrypt::InspectSaveFile(entry.path().string(), slot, isSystem, displayName, &meta);
                        std::string targetName = P4GCrypt::FormatPcSaveName(slot, isSystem);

                        std::string extra = isSystem ? "Données Système (system.bin)" :
                                            (!meta.protagonistName.empty() ?
                                                (meta.protagonistName + " (Niv. " + std::to_string(meta.level) + ")" +
                                                 (meta.isClear ? " [Terminé] - " : (" [" + meta.inGameDate + "] - ")) +
                                                 meta.playtimeFormatted) :
                                                ("Emplacement " + std::to_string(slot)));

                        m_detectedSaves.push_back({
                            displayName,
                            entry.path().string(),
                            entry.file_size(ec),
                            slot,
                            slot,
                            false,
                            isSystem,
                            extra,
                            targetName,
                            false,
                            false,
                            ""
                        });
                    }
                } else if (currentProfile.id == GameId::Persona5Royal) {
                    // P5R: matches data.dat, system.dat, cusa17416*, cusa17419*, data01..16, or .dat
                    bool isP5rFile = (lowerName.find("data.dat") != std::string::npos ||
                                      lowerName.find("system.dat") != std::string::npos ||
                                      lowerParent.find("cusa17416") != std::string::npos ||
                                      lowerParent.find("cusa17419") != std::string::npos ||
                                      lowerParent.find("data") != std::string::npos ||
                                      lowerParent.find("system") != std::string::npos ||
                                      ext == ".dat");

                    if (ext == ".bak" || ext == ".sfo" || ext == ".png" || ext == ".vdf") {
                        isP5rFile = false;
                    }

                    if (isP5rFile) {
                        int slot = 1;
                        bool isSystem = false;
                        std::string displayName;
                        P5RCrypt::P5RMetadata meta;
                        P5RCrypt::InspectSaveFile(entry.path().string(), slot, isSystem, displayName, &meta);
                        std::string targetName = P5RCrypt::FormatPcSavePath(slot, isSystem);

                        std::string extra = isSystem ? "Données Système (SYSTEM.DAT)" :
                                            (!meta.protagonistName.empty() ?
                                                (meta.protagonistName + " (Niv. " + std::to_string(meta.level) + ") - " + meta.playtimeFormatted) :
                                                ("Emplacement " + std::to_string(slot)));

                        m_detectedSaves.push_back({
                            displayName,
                            entry.path().string(),
                            entry.file_size(ec),
                            slot,
                            slot,
                            false,
                            isSystem,
                            extra,
                            targetName,
                            false,
                            false,
                            ""
                        });
                    }
                } else if (currentProfile.id == GameId::Persona5Strikers) {
                    // P5S: matches savedata.bin, app.bin, cusa1964*, or exact PS4/PC sizes
                    uintmax_t fsize = entry.file_size(ec);
                    bool isP5sFile = (lowerName == "savedata.bin" ||
                                      lowerName == "app.bin" ||
                                      lowerParent.find("cusa19641") != std::string::npos ||
                                      lowerParent.find("cusa19642") != std::string::npos ||
                                      lowerParent.find("cusa19643") != std::string::npos ||
                                      lowerParent.find("cusa19644") != std::string::npos ||
                                      lowerParent.find("cusa19645") != std::string::npos ||
                                      fsize == P5SCrypt::FORMAT_PS4_EN.size ||
                                      fsize == P5SCrypt::FORMAT_PS4_JP.size ||
                                      fsize == P5SCrypt::FORMAT_PC.size);

                    if (ext == ".bak" || ext == ".sfo" || ext == ".png" || ext == ".vdf") {
                        isP5sFile = false;
                    }

                    if (isP5sFile) {
                        uint64_t steamId = 0;
                        if (m_selectedSteamAccountIndex >= 0 && m_selectedSteamAccountIndex < static_cast<int>(m_steamAccounts.size())) {
                            try { steamId = std::stoull(m_steamAccounts[m_selectedSteamAccountIndex].steamId64); } catch (...) {}
                        }

                        P5SMetadata meta;
                        std::string displayName;
                        P5SCrypt::InspectSaveFile(entry.path().string(), steamId, meta, displayName);
                        std::string targetName = "SAVEDATA.BIN";

                        std::string extra;
                        int filledCount = 0;
                        std::string sampleHero;
                        for (const auto& s : meta.slots) {
                            if (s.hasData) {
                                filledCount++;
                                if (sampleHero.empty() && !s.fullName.empty()) {
                                    sampleHero = s.fullName + " (Niv. " + std::to_string(s.level) + ")";
                                }
                            }
                        }
                        if (filledCount > 0) {
                            extra = std::to_string(filledCount) + " slot(s) actif(s)";
                            if (!sampleHero.empty()) extra += " - " + sampleHero;
                        } else {
                            extra = "Conteneur 10 emplacements";
                        }

                        m_detectedSaves.push_back({
                            displayName,
                            entry.path().string(),
                            fsize,
                            1,
                            1,
                            false,
                            false,
                            extra,
                            targetName,
                            false,
                            false,
                            ""
                        });
                    }
                } else if (currentProfile.id == GameId::Persona5Tactica) {
                    // P5T: matches Data.dat, SaveData.dat, cusa43147, cusa43148, cusa35966, cusa43440, dec_100, dec_200, dec_300...
                    bool isP5tFile = (lowerName == "data.dat" || lowerName == "savedata.dat" ||
                                      lowerParent.find("cusa43147") != std::string::npos ||
                                      lowerParent.find("cusa43148") != std::string::npos ||
                                      lowerParent.find("cusa35966") != std::string::npos ||
                                      lowerParent.find("cusa43440") != std::string::npos ||
                                      lowerParent.find("dec_100") != std::string::npos ||
                                      lowerParent.find("dec_200") != std::string::npos ||
                                      lowerParent.find("dec_300") != std::string::npos);

                    if (lowerParent.find("400") != std::string::npos || lowerName.find("400") != std::string::npos ||
                        lowerParent.find("201") != std::string::npos || lowerName.find("201") != std::string::npos ||
                        lowerParent.find("100") != std::string::npos || lowerName.find("100") != std::string::npos) {
                        isP5tFile = false; // Les slots console 100 (système), 201 (DLC) et 400 (sauvegarde rapide) ne sont pas convertis
                    }

                    if (ext == ".bak" || ext == ".sfo" || ext == ".png" || ext == ".txt" || ext == ".vdf") {
                        isP5tFile = false;
                    }

                    if (isP5tFile && (lowerName == "data.dat" || lowerName == "savedata.dat")) {
                        P5TMetadata meta;
                        std::string displayName;
                        P5TCrypt::InspectSaveFile(entry.path().string(), meta, displayName);
                        if (meta.slotNumber == 400 || meta.dirName == "400" || meta.slotNumber == 201 || meta.dirName == "201") {
                            continue;
                        }
                        std::string targetName = meta.dirName + "/SaveData.dat";

                        std::string extra = meta.isSystem ? "Données Système (100)" :
                                            (meta.isAutoSave ? ("Sauvegarde Auto - " + meta.protagonistName + " (Niv. " + std::to_string(meta.teamLevel) + ") - " + meta.playtimeFormatted) :
                                             (meta.protagonistName + " (Niv. " + std::to_string(meta.teamLevel) + ") - " + meta.playtimeFormatted));

                        m_detectedSaves.push_back({
                            displayName,
                            entry.path().string(),
                            entry.file_size(ec),
                            meta.slotNumber,
                            meta.slotNumber,
                            false,
                            meta.isSystem,
                            extra,
                            targetName,
                            false,
                            false,
                            ""
                        });
                    }
                } else {
                    // P3R: matches ue4savegame.ps4*, *.sav, or files/folders named SaveData*
                    bool isP3rFile = (lowerName.find("ue4savegame") != std::string::npos ||
                                      ext == ".sav" ||
                                      lowerName.find("savedata") != std::string::npos ||
                                      lowerParent.find("savedata") != std::string::npos);

                    if (isP3rFile) {
                        int slot = 1;
                        bool isAigis = false;
                        bool isSystem = false;
                        std::string displayName;
                        P3RCrypt::InspectSaveFile(entry.path().string(), slot, isAigis, isSystem, displayName);
                        std::string targetName = P3RCrypt::FormatPcSaveName(slot, isAigis, isSystem);

                        m_detectedSaves.push_back({
                            displayName,
                            entry.path().string(),
                            entry.file_size(ec),
                            slot,
                            slot,
                            isAigis,
                            isSystem,
                            isAigis ? "Épisode Aigis" : (isSystem ? "Système" : "Épisode Normal"),
                            targetName,
                            false,
                            false,
                            ""
                        });
                    }
                }
            }
        }

        // Sort files logically
        if (currentProfile.id == GameId::Smt5Vengeance) {
            std::sort(m_detectedSaves.begin(), m_detectedSaves.end(), [](const auto& a, const auto& b) {
                if (a.isSystem != b.isSystem) return !a.isSystem; // GameSave first, SysSave last
                return a.targetSlot < b.targetSlot;
            });
            AddLog(std::to_string(m_detectedSaves.size()) + " sauvegarde(s) SMT5V détectée(s).", LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona3Portable) {
            std::sort(m_detectedSaves.begin(), m_detectedSaves.end(), [](const auto& a, const auto& b) {
                if (a.isSystem != b.isSystem) return !a.isSystem;
                return a.targetSlot < b.targetSlot;
            });
            AddLog(std::to_string(m_detectedSaves.size()) + " sauvegarde(s) P3P détectée(s).", LogLevel::Info);
        } else if (currentProfile.id == GameId::SoulHackers2) {
            std::sort(m_detectedSaves.begin(), m_detectedSaves.end(), [](const auto& a, const auto& b) {
                if (a.isSystem != b.isSystem) return !a.isSystem;
                return a.targetSlot < b.targetSlot;
            });
            AddLog(std::to_string(m_detectedSaves.size()) + " sauvegarde(s) SH2 détectée(s).", LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona4Golden) {
            std::sort(m_detectedSaves.begin(), m_detectedSaves.end(), [](const auto& a, const auto& b) {
                if (a.isSystem != b.isSystem) return !a.isSystem;
                return a.targetSlot < b.targetSlot;
            });
            AddLog(std::to_string(m_detectedSaves.size()) + " sauvegarde(s) P4G détectée(s).", LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona5Royal) {
            std::sort(m_detectedSaves.begin(), m_detectedSaves.end(), [](const auto& a, const auto& b) {
                if (a.isSystem != b.isSystem) return !a.isSystem;
                return a.targetSlot < b.targetSlot;
            });
            AddLog(std::to_string(m_detectedSaves.size()) + " sauvegarde(s) P5R détectée(s).", LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona5Strikers) {
            AddLog(std::to_string(m_detectedSaves.size()) + " conteneur(s) P5S détecté(s).", LogLevel::Info);
        } else if (currentProfile.id == GameId::Persona5Tactica) {
            std::sort(m_detectedSaves.begin(), m_detectedSaves.end(), [](const auto& a, const auto& b) {
                if (a.isSystem != b.isSystem) return !a.isSystem;
                return a.targetSlot < b.targetSlot;
            });
            AddLog(std::to_string(m_detectedSaves.size()) + " sauvegarde(s) P5T détectée(s).", LogLevel::Info);
        } else {
            std::sort(m_detectedSaves.begin(), m_detectedSaves.end(), [](const auto& a, const auto& b) {
                if (a.isAigis != b.isAigis) return !a.isAigis;
                if (a.isSystem != b.isSystem) return !a.isSystem;
                return a.targetSlot < b.targetSlot;
            });
            AddLog(std::to_string(m_detectedSaves.size()) + " sauvegarde(s) P3R détectée(s).", LogLevel::Info);
        }

    }
}

void MainWindow::RefreshSteamAccounts() {
    m_steamAccounts = SteamDetector::DetectAccounts();
    m_selectedSteamAccountIndex = 0;
    if (!m_steamAccounts.empty()) {
        const auto& activeAcc = m_steamAccounts[0];
        AddLog("Compte Steam détecté : " + activeAcc.personaName + " (ID: " + activeAcc.steamId32 + ")" + 
               (activeAcc.isActive ? " [Actif]" : ""), LogLevel::Success);
    } else {
        AddLog("Aucun compte Steam configuré automatiquement sur ce PC.", LogLevel::Warning);
    }
}

void MainWindow::AutoDetectPcPath() {
    char appDataPath[MAX_PATH] = {0};
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);

    char localAppDataPath[MAX_PATH] = {0};
    SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath);

    const auto& currentProfile = m_games[m_selectedGameIndex];
    std::string path;

    if (m_targetPlatform == TargetPlatform::Steam) {
        if (!m_steamAccounts.empty() && m_selectedSteamAccountIndex >= 0 && 
            m_selectedSteamAccountIndex < static_cast<int>(m_steamAccounts.size())) {
            const auto& account = m_steamAccounts[m_selectedSteamAccountIndex];
            path = SteamDetector::GetGameSavePath(currentProfile, account);
        } else {
            std::string steamDir = SteamDetector::GetSteamInstallPath();
            if (currentProfile.id == GameId::Persona4Golden) {
                path = steamDir + "\\userdata\\<Votre_Steam_ID>\\1113000\\remote";
            } else if (currentProfile.id == GameId::Persona3Portable) {
                path = steamDir + "\\userdata\\<Votre_Steam_ID>\\1809700\\remote";
            } else if (currentProfile.id == GameId::SoulHackers2) {
                path = std::string(appDataPath) + "\\SEGA\\SOULHACKERS2\\Steam\\<Votre_Steam_ID>\\SaveData";
            } else if (currentProfile.id == GameId::Persona3Reload) {
                path = std::string(appDataPath) + "\\SEGA\\P3R\\Steam\\<Votre_Steam_ID>";
            } else if (currentProfile.id == GameId::Smt5Vengeance) {
                path = std::string(appDataPath) + "\\SEGA\\SMT5V\\Steam\\<Votre_Steam_ID>";
            } else if (currentProfile.id == GameId::Persona5Royal) {
                path = std::string(appDataPath) + "\\SEGA\\P5R\\Steam\\<Votre_Steam_ID>";
            } else if (currentProfile.id == GameId::Persona5Strikers) {
                path = std::string(appDataPath) + "\\SEGA\\Steam\\P5S\\<Votre_Steam_ID>";
            } else if (currentProfile.id == GameId::Persona5Tactica) {
                path = std::string(appDataPath) + "\\SEGA\\P5T\\Steam\\<Votre_Steam_ID>\\Save";
            }
        }
    } else {
        path = std::string(localAppDataPath) + "\\Packages\\" + currentProfile.codeName + "...\\SystemAppData\\wgs";
    }

    strncpy_s(m_destPath, sizeof(m_destPath), path.c_str(), _TRUNCATE);
    AddLog("Dossier PC auto-détecté (" + currentProfile.codeName + ") : " + path, LogLevel::Info);
}

void MainWindow::StartChainExport() {
    if (m_isConverting) return;

    if (m_detectedSaves.empty()) {
        AddLog("Aucune sauvegarde à exporter. Veuillez sélectionner un fichier ou dossier source valide.", LogLevel::Warning);
        return;
    }

    if (strlen(m_destPath) == 0) {
        AddLog("Veuillez sélectionner un dossier de destination PC.", LogLevel::Warning);
        return;
    }

    for (auto& s : m_detectedSaves) {
        s.isProcessed = false;
        s.isSuccess = false;
        s.errorDetails.clear();
    }

    m_isConverting = true;
    m_currentExportIndex = 0;
    m_totalProgress = 0.0f;
    m_lastStepTime = std::chrono::steady_clock::now();

    AddLog("=== DÉBUT DE L'EXPORTATION EN CHAÎNE (" + std::to_string(m_detectedSaves.size()) + " fichiers) ===", LogLevel::Info);
    AddLog("Jeu : " + m_games[m_selectedGameIndex].name, LogLevel::Info);
    AddLog("Dossier de destination : " + std::string(m_destPath), LogLevel::Info);
}

void MainWindow::UpdateChainExport() {
    if (!m_isConverting) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastStepTime).count();

    // Step every 180ms for clean visualization and smooth progress
    if (elapsed > 180) {
        m_lastStepTime = now;

        if (m_currentExportIndex < m_detectedSaves.size()) {
            auto& save = m_detectedSaves[m_currentExportIndex];

            m_currentStatusText = "Exportation : " + save.filename + " -> " + save.targetFilename + " (" +
                                  std::to_string(m_currentExportIndex + 1) + "/" +
                                  std::to_string(m_detectedSaves.size()) + ")...";

            // Ensure destination directory exists
            std::error_code ec;
            fs::create_directories(m_destPath, ec);

            fs::path destFilePath = fs::path(m_destPath) / save.targetFilename;

            // Backup existing file if requested
            if (m_createBackup && fs::exists(destFilePath, ec)) {
                fs::path bakPath = destFilePath;
                bakPath += ".bak";
                fs::copy_file(destFilePath, bakPath, fs::copy_options::overwrite_existing, ec);
                AddLog("Copie de sauvegarde : " + save.targetFilename + ".bak créée.", LogLevel::Info);
            }

            bool ok = false;
            std::string err;
            std::string dlcNote;

            if (m_games[m_selectedGameIndex].id == GameId::Smt5Vengeance) {
                // SMT5V export: AES-256-ECB + SHA-1 recalculation + DLC cleaning
                SMT5VCrypt::DlcCleanResult dlcRes;
                ok = SMT5VCrypt::EncryptSaveFile(save.fullPath, destFilePath.string(), m_removeDlcFlags, dlcRes, err);
                if (m_removeDlcFlags && dlcRes.AnyCleared()) {
                    dlcNote = " [Flags DLC nettoyés]";
                }
            } else if (m_games[m_selectedGameIndex].id == GameId::Persona3Portable) {
                // P3P export: PS4 TLV conversion + mandatory .BINslot generation with MD5
                std::string slotPathStr;
                if (!save.isSystem) {
                    fs::path slotFilePath = fs::path(m_destPath) / (save.targetFilename + "slot");
                    slotPathStr = slotFilePath.string();
                    if (m_createBackup && fs::exists(slotFilePath, ec)) {
                        fs::path bakPath = slotFilePath;
                        bakPath += ".bak";
                        fs::copy_file(slotFilePath, bakPath, fs::copy_options::overwrite_existing, ec);
                    }
                }
                ok = P3PCrypt::ConvertSaveFile(save.fullPath, destFilePath.string(), slotPathStr, save.targetSlot, err);
                if (ok && !save.isSystem) {
                    dlcNote = " [.BINslot généré avec MD5]";
                }
            } else if (m_games[m_selectedGameIndex].id == GameId::SoulHackers2) {
                // SH2 export: direct copy into SAVE##/data.dat + system.dat + steam_autocloud.vdf
                // Detect Steam ID from destination directory
                std::string steamId = SH2Crypt::DetectSteamId(m_destPath);

                // Determine save type from extra details or target name
                bool isSys = save.isSystem;
                bool isSys2 = (save.targetFilename.find("SYSTEM2") != std::string::npos);
                bool isAuto = (save.targetFilename.find("AUTO") != std::string::npos);

                // Backup existing directory if present
                std::string targetDirName = SH2Crypt::FormatPcDirName(save.targetSlot, isSys, isSys2, isAuto);
                fs::path targetDirPath = fs::path(m_destPath) / targetDirName;
                if (m_createBackup && fs::exists(targetDirPath / "data.dat", ec)) {
                    fs::path bakPath = targetDirPath / "data.dat.bak";
                    fs::copy_file(targetDirPath / "data.dat", bakPath, fs::copy_options::overwrite_existing, ec);
                    AddLog("Copie de sauvegarde : " + targetDirName + "/data.dat.bak créée.", LogLevel::Info);
                }

                bool dlcCleaned = false;
                ok = SH2Crypt::ConvertSaveFile(save.fullPath, m_destPath, save.targetSlot, isSys, isSys2, isAuto, steamId, err, m_removeDlcFlags, &dlcCleaned);
                if (ok) {
                    dlcNote = " [" + targetDirName + "/data.dat + system.dat]";
                    if (m_removeDlcFlags && dlcCleaned) {
                        dlcNote += " [Flags DLC nettoyés]";
                    }
                }
            } else if (m_games[m_selectedGameIndex].id == GameId::Persona4Golden) {
                // P4G export: binary copy + companion .binslot generation with dual-MD5 and P4GOLDEN signature
                std::string slotPathStr = (fs::path(m_destPath) / (save.targetFilename + "slot")).string();
                if (m_createBackup && fs::exists(slotPathStr, ec)) {
                    fs::path bakPath = slotPathStr;
                    bakPath += ".bak";
                    fs::copy_file(slotPathStr, bakPath, fs::copy_options::overwrite_existing, ec);
                }
                ok = P4GCrypt::ConvertSaveFile(save.fullPath, destFilePath.string(), slotPathStr, save.targetSlot, err);
                if (ok) {
                    dlcNote = " [.binslot généré avec MD5 & Signature P4GOLDEN]";
                }
            } else if (m_games[m_selectedGameIndex].id == GameId::Persona5Royal) {
                // P5R export: AES-256-CBC + zlib autonomous conversion
                if (m_createBackup && fs::exists(destFilePath, ec)) {
                    fs::path bakPath = destFilePath;
                    bakPath += ".bak";
                    fs::copy_file(destFilePath, bakPath, fs::copy_options::overwrite_existing, ec);
                }
                ok = P5RCrypt::ConvertSaveFile(save.fullPath, destFilePath.string(), save.targetSlot, err);
                if (ok) {
                    dlcNote = " [Chiffré AES-256-CBC + zlib]";
                }
            } else if (m_games[m_selectedGameIndex].id == GameId::Persona5Strikers) {
                // P5S export: container conversion + LCG XOR SteamID encryption
                uint64_t steamId = 0;
                if (m_selectedSteamAccountIndex >= 0 && m_selectedSteamAccountIndex < static_cast<int>(m_steamAccounts.size())) {
                    try { steamId = std::stoull(m_steamAccounts[m_selectedSteamAccountIndex].steamId64); } catch (...) {}
                }
                if (steamId == 0) {
                    // Try detect from dest path: %APPDATA%\SEGA\Steam\P5S\<SteamID32>
                    fs::path p(m_destPath);
                    std::string lastDir = p.filename().string();
                    if (lastDir.empty()) lastDir = p.parent_path().filename().string();
                    bool allDigit = !lastDir.empty();
                    for (char c : lastDir) {
                        if (!std::isdigit(static_cast<unsigned char>(c))) { allDigit = false; break; }
                    }
                    if (allDigit) {
                        try {
                            uint32_t id32 = static_cast<uint32_t>(std::stoul(lastDir));
                            steamId = 76561197960265728ULL + id32;
                        } catch (...) {}
                    }
                }

                if (m_createBackup && fs::exists(destFilePath, ec)) {
                    fs::path bakPath = destFilePath;
                    bakPath += ".bak";
                    fs::copy_file(destFilePath, bakPath, fs::copy_options::overwrite_existing, ec);
                }
                ok = P5SCrypt::ConvertSaveFile(save.fullPath, destFilePath.string(), steamId, err);
                if (ok) {
                    dlcNote = " [Chiffré LCG SteamID: " + std::to_string(steamId) + "]";
                }
            } else if (m_games[m_selectedGameIndex].id == GameId::Persona5Tactica) {
                // If this is system save (100) and PC already has a 100 save, preserve PC graphics and controls!
                if (save.isSystem) {
                    AddLog("[P5T Save] Paramètres PC (100) conservés (résolution DirectX et contrôles PC protégés).", LogLevel::Info);
                    ok = true;
                    dlcNote = " [Paramètres PC préservés]";
                } else {
                    // P5T export: SaveData.dat + generation of SaveInfo.dat in <m_destPath>/<slotDir>
                    std::filesystem::path targetSubDir = fs::path(m_destPath) / std::to_string(save.targetSlot);
                    std::filesystem::path targetDataFile = targetSubDir / "SaveData.dat";
                    if (m_createBackup && fs::exists(targetDataFile, ec)) {
                        fs::path bakPath = targetDataFile;
                        bakPath += ".bak";
                        fs::copy_file(targetDataFile, bakPath, fs::copy_options::overwrite_existing, ec);
                        AddLog("Copie de sauvegarde : " + targetSubDir.filename().string() + "/SaveData.dat.bak créée.", LogLevel::Info);
                    }
                    ok = P5TCrypt::ConvertSaveFile(save.fullPath, m_destPath, save.targetSlot, err);
                    if (ok) {
                        dlcNote = " [SaveData.dat + SaveInfo.dat]";
                    }
                }
            } else {
                // P3R export: P3RCrypt XOR/bitshift + DLC cleaning
                P3RCrypt::DlcCleanResult dlcRes;
                ok = P3RCrypt::EncryptSaveFile(save.fullPath, destFilePath.string(), m_removeDlcFlags, dlcRes, err);
                if (m_removeDlcFlags && dlcRes.AnyCleared()) {
                    dlcNote = " [DLC PS4 retiré]";
                }
            }

            save.isProcessed = true;

            if (ok) {
                save.isSuccess = true;
                std::string tag = save.isSystem ? "[Système]" :
                                  (m_games[m_selectedGameIndex].id == GameId::Smt5Vengeance ? "[GameSave]" :
                                   (m_games[m_selectedGameIndex].id == GameId::Persona3Portable ? "[P3P Save]" :
                                    (m_games[m_selectedGameIndex].id == GameId::SoulHackers2 ? "[SH2 Save]" :
                                     (m_games[m_selectedGameIndex].id == GameId::Persona4Golden ? "[P4G Save]" :
                                      (m_games[m_selectedGameIndex].id == GameId::Persona5Royal ? "[P5R Save]" :
                                       (m_games[m_selectedGameIndex].id == GameId::Persona5Strikers ? "[P5S Save]" :
                                        (m_games[m_selectedGameIndex].id == GameId::Persona5Tactica ? "[P5T Save]" :
                                         (save.isAigis ? "[Épisode Aigis]" : "[Épisode Normal]"))))))));

                AddLog(tag + " " + save.filename + " -> " + save.targetFilename + " traité avec succès !" + dlcNote, LogLevel::Success);
            } else {
                save.isSuccess = false;
                save.errorDetails = err;
                AddLog("[ERREUR] Échec sur " + save.filename + " : " + err, LogLevel::Error);
            }

            m_currentExportIndex++;
            m_totalProgress = static_cast<float>(m_currentExportIndex) / static_cast<float>(m_detectedSaves.size());
        } else {
            m_isConverting = false;
            m_totalProgress = 1.0f;

            size_t successCount = 0;
            for (const auto& s : m_detectedSaves) {
                if (s.isSuccess) successCount++;
            }

            if (m_games[m_selectedGameIndex].id == GameId::Persona3Portable) {
                if (P3PCrypt::UpdateSteamRemoteCache(m_destPath)) {
                    AddLog("Fichier Steam remotecache.vdf synchronisé avec succès pour P3P.", LogLevel::Success);
                }
            } else if (m_games[m_selectedGameIndex].id == GameId::Persona4Golden) {
                if (P4GCrypt::UpdateSteamRemoteCache(m_destPath)) {
                    AddLog("Fichier Steam remotecache.vdf synchronisé avec succès pour P4G.", LogLevel::Success);
                }
            }

            m_currentStatusText = "Export terminé : " + std::to_string(successCount) + "/" + std::to_string(m_detectedSaves.size()) + " sauvegardes traitées avec succès.";
            AddLog("=== EXPORT EN CHAÎNE TERMINÉ AVEC SUCCÈS (" + std::to_string(successCount) + " sauvegardes prêtes sur PC) ===", LogLevel::Success);
        }
    }
}

void MainWindow::Render() {
    UpdateChainExport();

    // Main Fullscreen ImGui Window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));

    ImGui::Begin("AtlusSaveConverterMainWindow", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // Menu Bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Fichier")) {
            if (ImGui::MenuItem("Actualiser le dossier source", "F5")) { ScanSourceSaves(); }
            if (ImGui::MenuItem("Auto-détecter chemin PC", "Ctrl+D")) { AutoDetectPcPath(); }
            ImGui::Separator();
            if (ImGui::MenuItem("Quitter", "Alt+F4")) { PostQuitMessage(0); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Jeux Atlus")) {
            for (size_t i = 0; i < m_games.size(); ++i) {
                if (m_games[i].isAvailable) {
                    bool isSelected = (static_cast<int>(i) == m_selectedGameIndex);
                    if (ImGui::MenuItem(m_games[i].name.c_str(), nullptr, isSelected)) {
                        SetSelectedGame(static_cast<int>(i));
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    const auto& currentProfile = m_games[m_selectedGameIndex];

    // Top Header Banner with Game Selector Tabs
    ImGui::BeginChild("HeaderBanner", ImVec2(0, 50), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::TextColored(ThemeManager::PrimaryColor, "AtlusSaveConverter");
    ImGui::SameLine();
    ImGui::TextDisabled("v1.3.0");
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // Game Selector Buttons
    for (size_t i = 0; i < m_games.size(); ++i) {
        const auto& g = m_games[i];
        ImGui::PushID(static_cast<int>(i));

        if (g.isAvailable) {
            bool isCurrent = (static_cast<int>(i) == m_selectedGameIndex);
            if (isCurrent) {
                ImGui::PushStyleColor(ImGuiCol_Button, ThemeManager::PrimaryColor);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.08f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.20f, 0.25f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.90f, 1.0f));
            }

            if (ImGui::Button(g.codeName.c_str(), ImVec2(75, 26))) {
                SetSelectedGame(static_cast<int>(i));
            }
            ImGui::PopStyleColor(2);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.15f, 0.4f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.50f, 1.0f));
            std::string label = g.codeName + " (TODO)";
            ImGui::Button(label.c_str(), ImVec2(95, 26));
            ImGui::PopStyleColor(2);
        }

        ImGui::SameLine();
        ImGui::PopID();
    }

    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (currentProfile.id == GameId::Smt5Vengeance) {
        ImGui::TextColored(ThemeManager::AccentColor, "Moteur : SMT5VCrypt (AES-256-ECB & SHA-1)");
    } else if (currentProfile.id == GameId::Persona3Portable) {
        ImGui::TextColored(ThemeManager::AccentColor, "Moteur : P3PCrypt (TLV PS4->PC & MD5 .BINslot)");
    } else if (currentProfile.id == GameId::SoulHackers2) {
        ImGui::TextColored(ThemeManager::AccentColor, "Moteur : SH2 Direct Copy (Structure SAVE## & system.dat)");
    } else if (currentProfile.id == GameId::Persona4Golden) {
        ImGui::TextColored(ThemeManager::AccentColor, "Moteur : P4GCrypt (MD5 & Signature P4GOLDEN .binslot)");
    } else {
        ImGui::TextColored(ThemeManager::AccentColor, "Moteur : P3RCrypt (Clé Atlus XOR/Bitshift)");
    }

    ImGui::EndChild();

    ImGui::Spacing();

    // Two Clean Columns: Left Source, Right Destination
    float contentWidth = ImGui::GetContentRegionAvail().x;
    float colWidth = (contentWidth - 14.0f) * 0.5f;

    ImGui::Columns(2, "MainColumns", false);
    ImGui::SetColumnWidth(0, colWidth + 7.0f);
    ImGui::SetColumnWidth(1, colWidth + 7.0f);

    // ==========================================
    // COLONNE 1 : SOURCE PS4
    // ==========================================
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ThemeManager::CardBgColor);
    ImGui::PushStyleColor(ImGuiCol_Border, ThemeManager::CardBorderColor);
    ImGui::BeginChild("SourcePanel", ImVec2(0, 315), true);

    ImGui::TextColored(ThemeManager::AccentColor, "SOURCE : SAUVEGARDES PS4 (%s)", currentProfile.codeName.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Fichier ou dossier source dumpé (Apollo / PS4) :");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 170.0f);
    if (ImGui::InputText("##SourcePath", m_sourcePath, sizeof(m_sourcePath))) {
        ScanSourceSaves();
    }

    ImGui::SameLine();
    if (ImGui::Button("Fichier...##BrowseFile", ImVec2(75, 0))) {
        std::string selected;
        if (NativeDialogs::SelectFile(selected, L"Sélectionner un fichier de sauvegarde")) {
            strncpy_s(m_sourcePath, sizeof(m_sourcePath), selected.c_str(), _TRUNCATE);
            ScanSourceSaves();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Dossier...##BrowseDir", ImVec2(80, 0))) {
        std::string selected;
        if (NativeDialogs::SelectFolder(selected, L"Sélectionner le dossier des sauvegardes")) {
            strncpy_s(m_sourcePath, sizeof(m_sourcePath), selected.c_str(), _TRUNCATE);
            ScanSourceSaves();
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Sauvegardes détectées (%d fichier(s)) :", (int)m_detectedSaves.size());

    // Clean list of detected saves
    ImGui::BeginChild("DetectedSavesBox", ImVec2(0, 175), true);
    if (m_detectedSaves.empty()) {
        ImGui::TextDisabled("Aucun fichier détecté. Renseignez un fichier ou un dossier source ci-dessus.");
    } else {
        for (size_t i = 0; i < m_detectedSaves.size(); ++i) {
            auto& save = m_detectedSaves[i];
            ImGui::PushID(static_cast<int>(i));

            // Status Indicator
            if (save.isProcessed) {
                if (save.isSuccess) {
                    ImGui::TextColored(ThemeManager::SuccessColor, "[OK]");
                } else {
                    ImGui::TextColored(ThemeManager::ErrorColor, "[ERR]");
                }
            } else {
                ImGui::TextColored(ThemeManager::AccentColor, "[*]");
            }
            ImGui::SameLine();

            // Type / Episode Badge
            if (currentProfile.id == GameId::Smt5Vengeance) {
                if (save.isSystem) {
                    ImGui::TextDisabled("[SysSave]");
                } else {
                    ImGui::TextColored(ThemeManager::PrimaryColor, "[GameSave]");
                }
            } else if (currentProfile.id == GameId::Persona3Portable) {
                if (save.isSystem) {
                    ImGui::TextDisabled("[SYSTEM]");
                } else {
                    ImGui::TextColored(ThemeManager::PrimaryColor, "[P3P Save]");
                }
            } else if (currentProfile.id == GameId::SoulHackers2) {
                if (save.isSystem) {
                    ImGui::TextDisabled("[SysData]");
                } else if (save.targetFilename.find("AUTO") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "[AutoSave]");
                } else {
                    ImGui::TextColored(ThemeManager::PrimaryColor, "[SH2 Save]");
                }
            } else if (currentProfile.id == GameId::Persona4Golden) {
                if (save.isSystem) {
                    ImGui::TextDisabled("[SYSTEM]");
                } else {
                    ImGui::TextColored(ThemeManager::PrimaryColor, "[P4G Save]");
                }
            } else if (currentProfile.id == GameId::Persona5Royal) {
                if (save.isSystem) {
                    ImGui::TextDisabled("[SYSTEM]");
                } else {
                    ImGui::TextColored(ThemeManager::PrimaryColor, "[P5R Save]");
                }
            } else if (currentProfile.id == GameId::Persona5Strikers) {
                ImGui::TextColored(ThemeManager::PrimaryColor, "[P5S Save]");
            } else if (currentProfile.id == GameId::Persona5Tactica) {
                if (save.isSystem) {
                    ImGui::TextDisabled("[Système]");
                } else if (save.targetSlot == 200 || save.targetSlot == 201) {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "[AutoSave]");
                } else {
                    ImGui::TextColored(ThemeManager::PrimaryColor, "[P5T Save]");
                }
            } else {
                if (save.isAigis) {
                    ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "[Aigis]");
                } else if (save.isSystem) {
                    ImGui::TextDisabled("[System]");
                } else {
                    ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "[Normal]");
                }
            }

            ImGui::SameLine();
            ImGui::Text("%s", save.filename.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", save.extraDetails.c_str());

            // Slot changer for regular saves (not system files, not monolithic containers like P5S, not auto-saves)
            if (!save.isSystem && currentProfile.id != GameId::Persona5Strikers && !(currentProfile.id == GameId::Persona5Tactica && (save.targetSlot == 200 || save.targetSlot == 201))) {
                ImGui::SameLine(ImGui::GetWindowWidth() - 250);
                ImGui::TextDisabled("Slot cible :");
                ImGui::SameLine();
                ImGui::Text("%d", save.targetSlot);
                ImGui::SameLine();
                if (ImGui::SmallButton("-")) {
                    if (currentProfile.id == GameId::Smt5Vengeance) {
                        if (save.targetSlot > 1) {
                            save.targetSlot--;
                            save.targetFilename = SMT5VCrypt::FormatPcSaveName(save.targetSlot, save.isSystem);
                        }
                    } else if (currentProfile.id == GameId::Persona3Portable) {
                        if (save.targetSlot > 1) {
                            save.targetSlot--;
                            char buf[64];
                            std::snprintf(buf, sizeof(buf), "P3PSAVE%04d.BIN", save.targetSlot);
                            save.targetFilename = buf;
                        }
                    } else if (currentProfile.id == GameId::SoulHackers2) {
                        if (save.targetSlot > 1 && save.targetFilename.find("AUTO") == std::string::npos) {
                            save.targetSlot--;
                            bool isSys2 = (save.targetFilename.find("SYSTEM2") != std::string::npos);
                            save.targetFilename = SH2Crypt::FormatPcDirName(save.targetSlot, save.isSystem, isSys2, false);
                        }
                    } else if (currentProfile.id == GameId::Persona4Golden) {
                        if (save.targetSlot > 1) {
                            save.targetSlot--;
                            save.targetFilename = P4GCrypt::FormatPcSaveName(save.targetSlot, save.isSystem);
                        }
                    } else if (currentProfile.id == GameId::Persona5Royal) {
                        if (save.targetSlot > 1) {
                            save.targetSlot--;
                            save.targetFilename = P5RCrypt::FormatPcSavePath(save.targetSlot, save.isSystem);
                        }
                    } else if (currentProfile.id == GameId::Persona5Tactica) {
                        if (save.targetSlot > 300) {
                            save.targetSlot--;
                            save.targetFilename = std::to_string(save.targetSlot) + "/SaveData.dat";
                        }
                    } else {
                        if (save.targetSlot > (save.isAigis ? 1001 : 1)) {
                            save.targetSlot--;
                            save.targetFilename = P3RCrypt::FormatPcSaveName(save.targetSlot, save.isAigis, save.isSystem);
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("+")) {
                    save.targetSlot++;
                    if (currentProfile.id == GameId::Smt5Vengeance) {
                        save.targetFilename = SMT5VCrypt::FormatPcSaveName(save.targetSlot, save.isSystem);
                    } else if (currentProfile.id == GameId::Persona3Portable) {
                        char buf[64];
                        std::snprintf(buf, sizeof(buf), "P3PSAVE%04d.BIN", save.targetSlot);
                        save.targetFilename = buf;
                    } else if (currentProfile.id == GameId::SoulHackers2) {
                        if (save.targetFilename.find("AUTO") == std::string::npos) {
                            bool isSys2 = (save.targetFilename.find("SYSTEM2") != std::string::npos);
                            save.targetFilename = SH2Crypt::FormatPcDirName(save.targetSlot, save.isSystem, isSys2, false);
                        }
                    } else if (currentProfile.id == GameId::Persona4Golden) {
                        save.targetFilename = P4GCrypt::FormatPcSaveName(save.targetSlot, save.isSystem);
                    } else if (currentProfile.id == GameId::Persona5Royal) {
                        save.targetFilename = P5RCrypt::FormatPcSavePath(save.targetSlot, save.isSystem);
                    } else if (currentProfile.id == GameId::Persona5Tactica) {
                        save.targetFilename = std::to_string(save.targetSlot) + "/SaveData.dat";
                    } else {
                        save.targetFilename = P3RCrypt::FormatPcSaveName(save.targetSlot, save.isAigis, save.isSystem);
                    }
                }
            }

            ImGui::SameLine(ImGui::GetWindowWidth() - 85);
            ImGui::TextDisabled("%llu Ko", save.sizeBytes / 1024);

            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::PopStyleColor(2);

    ImGui::NextColumn();

    // ==========================================
    // COLONNE 2 : DESTINATION PC
    // ==========================================
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ThemeManager::CardBgColor);
    ImGui::PushStyleColor(ImGuiCol_Border, ThemeManager::CardBorderColor);
    ImGui::BeginChild("DestPanel", ImVec2(0, 315), true);

    ImGui::TextColored(ThemeManager::AccentColor, "DESTINATION : DOSSIER PC (%s)", currentProfile.codeName.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Plateforme PC cible :");
    if (ImGui::RadioButton("Steam (Chiffrement automatique)", m_targetPlatform == TargetPlatform::Steam)) {
        m_targetPlatform = TargetPlatform::Steam;
        AutoDetectPcPath();
    }
    ImGui::SameLine(240);
    if (ImGui::RadioButton("Xbox Game Pass", m_targetPlatform == TargetPlatform::XboxGamePass)) {
        m_targetPlatform = TargetPlatform::XboxGamePass;
        AutoDetectPcPath();
    }

    // Sélecteur de Compte Steam
    if (m_targetPlatform == TargetPlatform::Steam) {
        ImGui::Spacing();
        ImGui::Text("Compte Steam cible :");
        if (!m_steamAccounts.empty()) {
            std::string previewText;
            if (m_selectedSteamAccountIndex >= 0 && m_selectedSteamAccountIndex < static_cast<int>(m_steamAccounts.size())) {
                const auto& acc = m_steamAccounts[m_selectedSteamAccountIndex];
                previewText = acc.personaName + " (" + acc.steamId32 + ")" + (acc.isActive ? " [Actif]" : "");
            } else {
                previewText = "Sélectionner un compte...";
            }

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.0f);
            if (ImGui::BeginCombo("##SteamAccountCombo", previewText.c_str())) {
                for (int i = 0; i < static_cast<int>(m_steamAccounts.size()); ++i) {
                    const auto& acc = m_steamAccounts[i];
                    bool isSelected = (i == m_selectedSteamAccountIndex);
                    std::string itemLabel = acc.personaName + " (ID: " + acc.steamId32 + ")" + (acc.isActive ? "  [Actif]" : "");
                    if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                        m_selectedSteamAccountIndex = i;
                        AutoDetectPcPath();
                        AddLog("Compte Steam sélectionné : " + acc.personaName + " (ID: " + acc.steamId32 + ")", LogLevel::Info);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Scanner##RefreshSteam", ImVec2(80, 0))) {
                RefreshSteamAccounts();
                AutoDetectPcPath();
            }
        } else {
            ImGui::TextDisabled("Aucun compte Steam détecté.");
            ImGui::SameLine();
            if (ImGui::Button("Scanner##RefreshSteam", ImVec2(80, 0))) {
                RefreshSteamAccounts();
                AutoDetectPcPath();
            }
        }
    }

    ImGui::Spacing();

    ImGui::Text("Dossier PC cible :");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 170.0f);
    ImGui::InputText("##DestPath", m_destPath, sizeof(m_destPath));

    ImGui::SameLine();
    if (ImGui::Button("Parcourir...##BrowseDest", ImVec2(80, 0))) {
        std::string selected;
        if (NativeDialogs::SelectFolder(selected, L"Sélectionner le dossier PC")) {
            strncpy_s(m_destPath, sizeof(m_destPath), selected.c_str(), _TRUNCATE);
            AddLog("Dossier PC sélectionné : " + selected, LogLevel::Info);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Auto##AutoDest", ImVec2(75, 0))) {
        AutoDetectPcPath();
    }

    ImGui::Spacing();
    ImGui::Checkbox("Créer une sauvegarde de sécurité (.bak) avant écrasement", &m_createBackup);
    ImGui::Checkbox("Retirer les flags DLC PS4 (Fix 'DLC manquant' sur PC)", &m_removeDlcFlags);

    ImGui::Spacing();
    ImGui::TextDisabled("Règles appliquées automatiquement :");
    if (currentProfile.id == GameId::Smt5Vengeance) {
        ImGui::BulletText("Sauvegardes de jeu : Renommé en GameSave[slot].sav (ex: GameSave01.sav)");
        ImGui::BulletText("Paramètres Système : Renommé en SysSave.sav");
        ImGui::BulletText("Chiffrement        : AES-256-ECB matériel avec calcul du checksum SHA-1");
        ImGui::BulletText("Patch DLC PS4      : Désactive les flags DLC à 0x529 pour compatibilité PC");
    } else if (currentProfile.id == GameId::Persona3Portable) {
        ImGui::BulletText("Sauvegardes de jeu : Renommé en P3PSAVE[slot].BIN (ex: P3PSAVE0001.BIN)");
        ImGui::BulletText("Paramètres Système : Renommé en SYSTEM.BIN");
        ImGui::BulletText("Fichier compagnon  : P3PSAVE[slot].BINslot généré avec MD5 valide");
        ImGui::BulletText("Dossier Steam      : userdata/<SteamID>/1809700/remote");
    } else if (currentProfile.id == GameId::SoulHackers2) {
        ImGui::BulletText("Sauvegardes de jeu : Structure de dossier SAVE[slot] avec data.dat");
        ImGui::BulletText("Métadonnées        : Génération automatique de system.dat (UTF-16LE JSON)");
        ImGui::BulletText("Steam Cloud        : Génération automatique de steam_autocloud.vdf");
        ImGui::BulletText("Dossier Steam      : %%APPDATA%%\\SEGA\\SOULHACKERS2\\Steam\\<SteamID>\\SaveData");
    } else if (currentProfile.id == GameId::Persona4Golden) {
        ImGui::BulletText("Sauvegardes de jeu : Renommé en data[slot].bin (ex: data0001.bin)");
        ImGui::BulletText("Paramètres Système : Renommé en system.bin");
        ImGui::BulletText("Fichier compagnon  : data[slot].binslot généré avec MD5 & Signature P4GOLDEN");
        ImGui::BulletText("Dossier Steam      : userdata/<SteamID>/1113000/remote");
    } else {
        ImGui::BulletText("Épisode Normal     : Renommé en SaveData[slot].sav (ex: SaveData002.sav)");
        ImGui::BulletText("Épisode Aigis      : Renommé en SaveData[slot].sav (ex: SaveData1002.sav)");
        ImGui::BulletText("Chiffrement        : Effectué via l'algorithme P3RCrypt (clé intégrée)");
        ImGui::BulletText("Patch DLC PS4      : Désactive les flags DLC PS4 pour compatibilité PC");
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(2);

    ImGui::Columns(1);
    ImGui::Spacing();

    // ==========================================
    // ACTION CENTRALE : EXPORT EN CHAÎNE
    // ==========================================
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 0.70f));
    ImGui::BeginChild("ActionCard", ImVec2(0, 80), true);

    float availWidth = ImGui::GetContentRegionAvail().x;
    float btnWidth = 440.0f;
    float btnHeight = 42.0f;

    ImGui::SetCursorPosX((availWidth - btnWidth) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ThemeManager::PrimaryColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeManager::PrimaryHoverColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeManager::PrimaryActiveColor);

    char btnText[128];
    if (m_isConverting) {
        sprintf_s(btnText, sizeof(btnText), "TRAITEMENT EN COURS... (%d/%d)", (int)m_currentExportIndex, (int)m_detectedSaves.size());
    } else {
        sprintf_s(btnText, sizeof(btnText), "EXPORTER ET TRAITER EN CHAÎNE (%d fichier(s))", (int)m_detectedSaves.size());
    }

    if (ImGui::Button(btnText, ImVec2(btnWidth, btnHeight))) {
        StartChainExport();
    }
    ImGui::PopStyleColor(3);

    if (m_isConverting) {
        ImGui::Spacing();
        ImGui::ProgressBar(m_totalProgress, ImVec2(-1.0f, 15.0f), m_currentStatusText.c_str());
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // ==========================================
    // CONSOLE DE JOURNALISATION
    // ==========================================
    ImGui::TextDisabled("JOURNAL D'ACTIVITÉ :");
    ImGui::SameLine(ImGui::GetWindowWidth() - 140);
    if (ImGui::SmallButton("Effacer le journal")) {
        m_logs.clear();
        AddLog("Journal effacé.", LogLevel::Info);
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.08f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.20f, 0.25f, 0.50f));

    float remainingHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("LogConsoleRegion", ImVec2(0, remainingHeight), true);

    for (const auto& log : m_logs) {
        ImGui::TextDisabled("[%s] ", log.timestamp.c_str());
        ImGui::SameLine();

        ImVec4 color = ImVec4(0.85f, 0.88f, 0.95f, 1.0f);
        switch (log.level) {
            case LogLevel::Info:
                color = ImVec4(0.75f, 0.85f, 0.98f, 1.0f);
                break;
            case LogLevel::Success:
                color = ThemeManager::SuccessColor;
                break;
            case LogLevel::Warning:
                color = ThemeManager::WarningColor;
                break;
            case LogLevel::Error:
                color = ThemeManager::ErrorColor;
                break;
        }

        ImGui::TextColored(color, "%s", log.message.c_str());
    }

    if (m_autoScrollLog && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(2);

    ImGui::End();
}

} // namespace Ps4Atlus
