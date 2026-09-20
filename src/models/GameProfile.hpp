#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Ps4Atlus {

enum class GameId {
    Persona3Reload,
    Smt5Vengeance,
    Persona3Portable,
    SoulHackers2,
    Persona5Royal,
    Persona4Golden,
    Persona5Strikers,
    Persona5Tactica
};

enum class GameRegion {
    USA,
    EUR,
    JPN,
    ASIA
};

enum class TargetPlatform {
    Steam,
    XboxGamePass
};

enum class EncryptionStatus {
    DecryptedValid,
    EncryptedPS4Raw,
    UnknownOrCorrupt,
    FileNotFound
};

struct SaveSlotInfo {
    int slotNumber = 1;
    std::string displayName;
    std::string internalFileName; // e.g. DATA0001.DAT, SaveData0001.sav, data0001.bin
    bool isSystemData = false;
    bool isQuickSave = false;
};

struct SaveMetadata {
    std::string protagonistName = "Joker";
    std::string inGameDate = "10/24 Mon";
    int playerLevel = 68;
    int difficultyLevel = 2; // 0: Safety, 1: Easy, 2: Normal, 3: Hard, 4: Merciless
    std::string difficultyName = "Normal";
    std::string playtimeFormatted = "78h 42m";
    uint64_t money = 1450200;
    uint64_t fileSizeBytes = 245760;
    std::string lastModified = "2024-03-12 21:30:15";
    EncryptionStatus status = EncryptionStatus::DecryptedValid;
    std::string detectedGameId = "CUSA17416";
    std::string checksumHex = "A4F82B910C6E";
    bool dlcPresent = true;
};

struct GameProfile {
    GameId id;
    std::string name;
    std::string subtitle;
    std::string codeName;
    std::string bannerColor;
    
    // Regions and PS4 Title IDs
    std::string titleIdUS;
    std::string titleIdEU;
    std::string titleIdJP;
    std::string titleIdAsia;

    // File naming conventions
    std::string saveFileExtension;
    std::string saveFilePattern; // e.g. "DATA{:04d}.DAT"
    std::string systemFileName;
    int maxSlots = 16;

    // Default paths on PC
    std::string defaultSteamPath;
    std::string defaultXboxPath;

    // Primary & Accent Colors (ImU32 / RGB)
    float primaryColor[4];
    float accentColor[4];
    float backgroundColor[4];

    bool isAvailable = true;
    std::string availabilityNote;
    std::string notes;
};

inline std::vector<GameProfile> GetDefaultProfiles() {
    std::vector<GameProfile> profiles;

    // 1. Persona 3 Reload (ACTIVE)
    {
        GameProfile p3r;
        p3r.id = GameId::Persona3Reload;
        p3r.name = "Persona 3 Reload";
        p3r.subtitle = "Tartarus Moonlight Save Exporter";
        p3r.codeName = "P3R";
        p3r.isAvailable = true;
        p3r.availabilityNote = "Actif";
        p3r.titleIdUS = "CUSA43887";
        p3r.titleIdEU = "CUSA43888";
        p3r.titleIdJP = "CUSA43885";
        p3r.titleIdAsia = "CUSA43886";
        p3r.saveFileExtension = ".sav";
        p3r.saveFilePattern = "SaveData%04d.sav";
        p3r.systemFileName = "SystemData.sav";
        p3r.maxSlots = 16;
        p3r.defaultSteamPath = "%LOCALAPPDATA%\\P3R\\Saved\\SaveGames\\<SteamID>";
        p3r.defaultXboxPath = "%LOCALAPPDATA%\\Packages\\SEGAofAmericaInc.L0...\\SystemAppData\\wgs";
        // Moonlight Blue
        p3r.primaryColor[0] = 0.00f; p3r.primaryColor[1] = 0.55f; p3r.primaryColor[2] = 0.95f; p3r.primaryColor[3] = 1.0f;
        p3r.accentColor[0] = 0.20f;  p3r.accentColor[1] = 0.75f;  p3r.accentColor[2] = 1.00f;  p3r.accentColor[3] = 1.0f;
        p3r.backgroundColor[0] = 0.05f; p3r.backgroundColor[1] = 0.08f; p3r.backgroundColor[2] = 0.15f; p3r.backgroundColor[3] = 1.0f;
        p3r.notes = "Sauvegarde Unreal Engine 4 (GVAS).";
        profiles.push_back(p3r);
    }

    // 2. Shin Megami Tensei V: Vengeance (ACTIVE)
    {
        GameProfile smt5v;
        smt5v.id = GameId::Smt5Vengeance;
        smt5v.name = "Shin Megami Tensei V: Vengeance";
        smt5v.subtitle = "Nahobino Canon Save Exporter";
        smt5v.codeName = "SMT5V";
        smt5v.isAvailable = true;
        smt5v.availabilityNote = "Actif";
        smt5v.titleIdUS = "CUSA42697";
        smt5v.titleIdEU = "CUSA42698";
        smt5v.titleIdJP = "CUSA42501";
        smt5v.titleIdAsia = "CUSA42502";
        smt5v.saveFileExtension = ".sav";
        smt5v.saveFilePattern = "GameSave%02d.sav";
        smt5v.systemFileName = "SysSave.sav";
        smt5v.maxSlots = 20;
        smt5v.defaultSteamPath = "%APPDATA%\\SEGA\\SMT5V\\Steam\\<SteamID>";
        smt5v.defaultXboxPath = "%LOCALAPPDATA%\\Packages\\SEGAofAmericaInc.SMT5V...\\SystemAppData\\wgs";
        // Nahobino Cyan & Gold
        smt5v.primaryColor[0] = 0.00f; smt5v.primaryColor[1] = 0.80f; smt5v.primaryColor[2] = 0.95f; smt5v.primaryColor[3] = 1.0f;
        smt5v.accentColor[0] = 1.00f;  smt5v.accentColor[1] = 0.82f;  smt5v.accentColor[2] = 0.10f;  smt5v.accentColor[3] = 1.0f;
        smt5v.backgroundColor[0] = 0.04f; smt5v.backgroundColor[1] = 0.08f; smt5v.backgroundColor[2] = 0.12f; smt5v.backgroundColor[3] = 1.0f;
        smt5v.notes = "Sauvegarde Unreal Engine 4 chiffrée AES-256-ECB.";
        profiles.push_back(smt5v);
    }

    // 3. Persona 3 Portable (FRONTEND)
    {
        GameProfile p3p;
        p3p.id = GameId::Persona3Portable;
        p3p.name = "Persona 3 Portable";
        p3p.subtitle = "Gekkoukan High School Save Exporter";
        p3p.codeName = "P3P";
        p3p.isAvailable = true;
        p3p.availabilityNote = "Actif";
        p3p.titleIdUS = "CUSA33871";
        p3p.titleIdEU = "CUSA33872";
        p3p.titleIdJP = "CUSA33880";
        p3p.titleIdAsia = "CUSA33885";
        p3p.saveFileExtension = ".BIN";
        p3p.saveFilePattern = "P3PSAVE%04d.BIN";
        p3p.systemFileName = "SYSTEM.BIN";
        p3p.maxSlots = 16;
        p3p.defaultSteamPath = "<SteamDir>\\userdata\\<SteamID>\\1809700\\remote";
        p3p.defaultXboxPath = "%LOCALAPPDATA%\\Packages\\SEGAofAmericaInc.Persona3Portable...\\SystemAppData\\wgs";
        // P3P Velvet Pink & Moonlight Cyan
        p3p.primaryColor[0] = 0.95f; p3p.primaryColor[1] = 0.22f; p3p.primaryColor[2] = 0.55f; p3p.primaryColor[3] = 1.0f;
        p3p.accentColor[0] = 0.25f;  p3p.accentColor[1] = 0.78f;  p3p.accentColor[2] = 1.00f;  p3p.accentColor[3] = 1.0f;
        p3p.backgroundColor[0] = 0.12f; p3p.backgroundColor[1] = 0.05f; p3p.backgroundColor[2] = 0.09f; p3p.backgroundColor[3] = 1.0f;
        p3p.notes = "Sauvegarde Atlus TLV avec génération obligatoire du fichier compagnon .BINslot et MD5.";
        profiles.push_back(p3p);
    }

    // 4. Soul Hackers 2 (ACTIVE)
    {
        GameProfile sh2;
        sh2.id = GameId::SoulHackers2;
        sh2.name = "Soul Hackers 2";
        sh2.subtitle = "Aion Agent Save Exporter";
        sh2.codeName = "SH2";
        sh2.isAvailable = true;
        sh2.availabilityNote = "Actif";
        sh2.titleIdUS = "CUSA27328";
        sh2.titleIdEU = "CUSA27329";
        sh2.titleIdJP = "CUSA27327";
        sh2.titleIdAsia = "CUSA27330";
        sh2.saveFileExtension = ".dat";
        sh2.saveFilePattern = "SAVE%02d";
        sh2.systemFileName = "SYSTEMDATA";
        sh2.maxSlots = 20;
        sh2.defaultSteamPath = "%APPDATA%\\\\SEGA\\\\SOULHACKERS2\\\\Steam\\\\<SteamID>\\\\SaveData";
        sh2.defaultXboxPath = "%LOCALAPPDATA%\\\\Packages\\\\SEGAofAmericaInc.SoulHackers2...\\\\SystemAppData\\\\wgs";
        // Neon Green & Cyan (Soul Hackers 2 style)
        sh2.primaryColor[0] = 0.00f; sh2.primaryColor[1] = 0.90f; sh2.primaryColor[2] = 0.55f; sh2.primaryColor[3] = 1.0f;
        sh2.accentColor[0] = 0.00f;  sh2.accentColor[1] = 0.85f;  sh2.accentColor[2] = 1.00f;  sh2.accentColor[3] = 1.0f;
        sh2.backgroundColor[0] = 0.04f; sh2.backgroundColor[1] = 0.06f; sh2.backgroundColor[2] = 0.10f; sh2.backgroundColor[3] = 1.0f;
        sh2.notes = "Aucun chiffrement. Copie directe des fichiers data.dat avec génération du system.dat (UTF-16LE JSON).";
        profiles.push_back(sh2);
    }

    // 5. Persona 5 Royal (ACTIVE)
    {
        GameProfile p5r;
        p5r.id = GameId::Persona5Royal;
        p5r.name = "Persona 5 Royal";
        p5r.subtitle = "The Phantom Thieves of Hearts";
        p5r.codeName = "P5R";
        p5r.isAvailable = true;
        p5r.availabilityNote = "Actif";
        p5r.titleIdUS = "CUSA17416";
        p5r.titleIdEU = "CUSA17419";
        p5r.titleIdJP = "CUSA08216";
        p5r.titleIdAsia = "CUSA17418";
        p5r.saveFileExtension = ".DAT";
        p5r.saveFilePattern = "DATA%02d";
        p5r.systemFileName = "SYSTEM.DAT";
        p5r.maxSlots = 16;
        p5r.defaultSteamPath = "%APPDATA%\\SEGA\\P5R\\Steam\\<SteamID>\\savedata";
        p5r.defaultXboxPath = "%LOCALAPPDATA%\\Packages\\SEGAofAmericaInc.F05A2528399_...\\SystemAppData\\wgs";
        // Crimson Red
        p5r.primaryColor[0] = 0.90f; p5r.primaryColor[1] = 0.07f; p5r.primaryColor[2] = 0.15f; p5r.primaryColor[3] = 1.0f;
        p5r.accentColor[0] = 1.00f;  p5r.accentColor[1] = 0.25f;  p5r.accentColor[2] = 0.25f;  p5r.accentColor[3] = 1.0f;
        p5r.backgroundColor[0] = 0.08f; p5r.backgroundColor[1] = 0.07f; p5r.backgroundColor[2] = 0.08f; p5r.backgroundColor[3] = 1.0f;
        p5r.notes = "Conversion PS4 déchiffrée vers PC (compression zlib + chiffrement AES-256-CBC officiel, 100% autonome sans param.sfo).";
        profiles.push_back(p5r);
    }


    // 6. Persona 4 Golden (ACTIVE)
    {
        GameProfile p4g;
        p4g.id = GameId::Persona4Golden;
        p4g.name = "Persona 4 Golden";
        p4g.subtitle = "Investigation Team of Inaba";
        p4g.codeName = "P4G";
        p4g.isAvailable = true;
        p4g.availabilityNote = "Actif";
        p4g.titleIdUS = "CUSA33887";
        p4g.titleIdEU = "CUSA33874";
        p4g.titleIdJP = "CUSA33886";
        p4g.titleIdAsia = "CUSA33889";
        p4g.saveFileExtension = ".bin";
        p4g.saveFilePattern = "data%04d.bin";
        p4g.systemFileName = "system.bin";
        p4g.maxSlots = 16;
        p4g.defaultSteamPath = "<SteamDir>\\userdata\\<SteamID>\\1113000\\remote";
        p4g.defaultXboxPath = "%LOCALAPPDATA%\\Packages\\SEGAofAmericaInc.Persona4Golden...\\SystemAppData\\wgs";
        // Amber Yellow
        p4g.primaryColor[0] = 0.95f; p4g.primaryColor[1] = 0.72f; p4g.primaryColor[2] = 0.00f; p4g.primaryColor[3] = 1.0f;
        p4g.accentColor[0] = 1.00f;  p4g.accentColor[1] = 0.85f;  p4g.accentColor[2] = 0.20f;  p4g.accentColor[3] = 1.0f;
        p4g.backgroundColor[0] = 0.12f; p4g.backgroundColor[1] = 0.10f; p4g.backgroundColor[2] = 0.06f; p4g.backgroundColor[3] = 1.0f;
        p4g.notes = "Sauvegarde PlayStation 4 déchiffrée avec génération obligatoire du fichier compagnon .binslot (MD5 + signature P4GOLDEN).";
        profiles.push_back(p4g);
    }

    // 7. Persona 5 Strikers (ACTIVE)
    {
        GameProfile p5s;
        p5s.id = GameId::Persona5Strikers;
        p5s.name = "Persona 5 Strikers";
        p5s.subtitle = "Phantom Strikers All-Out Action";
        p5s.codeName = "P5S";
        p5s.isAvailable = true;
        p5s.availabilityNote = "Actif";
        p5s.titleIdUS = "CUSA19641";
        p5s.titleIdEU = "CUSA19642";
        p5s.titleIdJP = "CUSA19643";
        p5s.titleIdAsia = "CUSA19644";
        p5s.saveFileExtension = ".BIN";
        p5s.saveFilePattern = "SAVEDATA.BIN";
        p5s.systemFileName = "";
        p5s.maxSlots = 10;
        p5s.defaultSteamPath = "%APPDATA%\\SEGA\\Steam\\P5S\\<SteamID>";
        p5s.defaultXboxPath = "";
        // Fire Red / White / Black
        p5s.primaryColor[0] = 0.95f; p5s.primaryColor[1] = 0.15f; p5s.primaryColor[2] = 0.20f; p5s.primaryColor[3] = 1.0f;
        p5s.accentColor[0] = 1.00f;  p5s.accentColor[1] = 0.40f;  p5s.accentColor[2] = 0.10f;  p5s.accentColor[3] = 1.0f;
        p5s.backgroundColor[0] = 0.08f; p5s.backgroundColor[1] = 0.07f; p5s.backgroundColor[2] = 0.08f; p5s.backgroundColor[3] = 1.0f;
        p5s.notes = "Conversion PS4 déchiffrée (APP.BIN) vers conteneur PC 10 slots (chiffrement LCG XOR salé avec SteamID).";
        profiles.push_back(p5s);
    }

    // 8. Persona 5 Tactica (ACTIVE)
    {
        GameProfile p5t;
        p5t.id = GameId::Persona5Tactica;
        p5t.name = "Persona 5 Tactica";
        p5t.subtitle = "Phantom Thieves Revolution Exporter";
        p5t.codeName = "P5T";
        p5t.isAvailable = true;
        p5t.availabilityNote = "Actif";
        p5t.titleIdUS = "CUSA43147";
        p5t.titleIdEU = "CUSA43148";
        p5t.titleIdJP = "CUSA35966";
        p5t.titleIdAsia = "CUSA43440";
        p5t.saveFileExtension = ".dat";
        p5t.saveFilePattern = "Data.dat";
        p5t.systemFileName = "100";
        p5t.maxSlots = 50;
        p5t.defaultSteamPath = "%APPDATA%\\SEGA\\P5T\\Steam\\<SteamID>\\Save";
        p5t.defaultXboxPath = "";
        // Revolution Crimson Red / Charcoal Black / Flag Gold
        p5t.primaryColor[0] = 0.90f; p5t.primaryColor[1] = 0.12f; p5t.primaryColor[2] = 0.17f; p5t.primaryColor[3] = 1.0f;
        p5t.accentColor[0] = 1.00f;  p5t.accentColor[1] = 0.75f;  p5t.accentColor[2] = 0.10f;  p5t.accentColor[3] = 1.0f;
        p5t.backgroundColor[0] = 0.09f; p5t.backgroundColor[1] = 0.08f; p5t.backgroundColor[2] = 0.09f; p5t.backgroundColor[3] = 1.0f;
        p5t.notes = "Conversion PS4 (Data.dat GZIP/MessagePack) vers PC Steam (SaveData.dat + génération SaveInfo.dat).";
        profiles.push_back(p5t);
    }

    return profiles;

}

} // namespace Ps4Atlus
