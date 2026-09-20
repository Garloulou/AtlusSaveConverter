#include "core/SteamDetector.hpp"
#include <iostream>
#include <cassert>

using namespace Ps4Atlus;

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ps4Atlus - Steam Detector Test        \n";
    std::cout << "========================================\n\n";

    // 1. Steam Install Path
    std::string steamPath = SteamDetector::GetSteamInstallPath();
    std::cout << "[TEST 1] Steam Install Path: " << steamPath << "\n";
    assert(!steamPath.empty());

    // 2. Detect Steam Accounts
    std::cout << "\n[TEST 2] Detecting Steam Accounts:\n";
    auto accounts = SteamDetector::DetectAccounts();
    std::cout << "  Nombre de comptes trouvés: " << accounts.size() << "\n";
    assert(!accounts.empty());

    bool foundGarloulou = false;
    for (const auto& a : accounts) {
        std::cout << "  - " << a.personaName << " (SteamID32: " << a.steamId32 << ", SteamID64: " << a.steamId64 << ")"
                  << (a.isActive ? " [Actif]" : "") << "\n";
        if (a.steamId32 == "1236673082") {
            foundGarloulou = true;
            assert(a.isActive);
            assert(a.personaName == "Garloulou");
        }
    }
    assert(foundGarloulou);
    std::cout << "  -> Compte Garloulou (1236673082) bien détecté et actif ! [OK]\n";

    // 3. Test GetGameSavePath
    std::cout << "\n[TEST 3] Generating Game Save Paths for Selected Account:\n";
    const auto& activeAcc = accounts[0];

    GameProfile p4g;
    p4g.id = GameId::Persona4Golden;
    std::string p4gPath = SteamDetector::GetGameSavePath(p4g, activeAcc);
    std::cout << "  P4G Path: " << p4gPath << "\n";
    assert(p4gPath.find("1236673082\\1113000\\remote") != std::string::npos);

    GameProfile p3p;
    p3p.id = GameId::Persona3Portable;
    std::string p3pPath = SteamDetector::GetGameSavePath(p3p, activeAcc);
    std::cout << "  P3P Path: " << p3pPath << "\n";
    assert(p3pPath.find("1236673082\\1809700\\remote") != std::string::npos);

    GameProfile sh2;
    sh2.id = GameId::SoulHackers2;
    std::string sh2Path = SteamDetector::GetGameSavePath(sh2, activeAcc);
    std::cout << "  SH2 Path: " << sh2Path << "\n";
    assert(sh2Path.find("1236673082\\SaveData") != std::string::npos);

    std::cout << "\n========================================\n";
    std::cout << "  TOUS LES TESTS STEAM DETECTOR OK !    \n";
    std::cout << "========================================\n";
    return 0;
}
