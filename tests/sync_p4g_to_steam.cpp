#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include "core/P4GCrypt.hpp"

namespace fs = std::filesystem;

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ps4Atlus - Sync P4G Saves to Steam   \n";
    std::cout << "========================================\n\n";

    std::vector<std::string> steamDirs = {
        "C:\\Program Files (x86)\\Steam\\userdata\\1236673082\\1113000\\remote",
        "C:\\Program Files (x86)\\Steam\\userdata\\1028152820\\1113000\\remote"
    };

    struct SaveMapping {
        std::string srcFolder;
        std::string outName;
        int slot;
        bool isSystem;
    };

    std::vector<SaveMapping> mappings = {
        {"Save/Persona 4 golden/ps4/dec_CHANNELSAVE_CUSA33874", "system.bin", 0, true},
        {"Save/Persona 4 golden/ps4/dec_SAVELIST0001_CUSA33874", "data0001.bin", 1, false},
        {"Save/Persona 4 golden/ps4/dec_SAVELIST0002_CUSA33874", "data0002.bin", 2, false},
        {"Save/Persona 4 golden/ps4/dec_SAVELIST0003_CUSA33874", "data0003.bin", 3, false},
        {"Save/Persona 4 golden/ps4/dec_SAVELIST0004_CUSA33874", "data0004.bin", 4, false},
        {"Save/Persona 4 golden/ps4/dec_SAVELIST0005_CUSA33874", "data0005.bin", 5, false},
        {"Save/Persona 4 golden/ps4/dec_SAVELIST0016_CUSA33874", "data0016.bin", 16, false}
    };

    for (const auto& steamDir : steamDirs) {
        if (!fs::exists(steamDir)) {
            std::cout << "[SKIP] Dossier introuvable : " << steamDir << "\n";
            continue;
        }

        std::cout << "[DEST] " << steamDir << "\n";
        for (const auto& m : mappings) {
            std::string srcBin = m.srcFolder + "/GAME.BIN";
            if (!fs::exists(srcBin)) {
                std::cout << "  [WARN] Source introuvable : " << srcBin << "\n";
                continue;
            }

            std::string outBin = steamDir + "/" + m.outName;
            std::string outSlot = steamDir + "/" + m.outName + "slot";
            std::string err;

            bool ok = Ps4Atlus::P4GCrypt::ConvertSaveFile(srcBin, outBin, outSlot, m.slot, err);
            if (!ok) {
                std::cout << "  [FAIL] " << m.outName << " : " << err << "\n";
            } else {
                std::cout << "  [OK] " << m.outName << " + " << m.outName << "slot convertis.\n";
            }
        }

        if (Ps4Atlus::P4GCrypt::UpdateSteamRemoteCache(steamDir)) {
            std::cout << "  -> remotecache.vdf mis a jour pour " << steamDir << "\n";
        } else {
            std::cout << "  -> Echec mise a jour remotecache.vdf\n";
        }
        std::cout << "\n";
    }

    std::cout << "Termine avec succes !\n";
    return 0;
}
