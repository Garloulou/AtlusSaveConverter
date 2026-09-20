#include "core/P5TCrypt.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace Ps4Atlus;
namespace fs = std::filesystem;

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ps4Atlus - Persona 5 Tactica Test     \n";
    std::cout << "========================================\n\n";

    // [TEST 1] GZIP Compression / Decompression Symmetry
    std::cout << "[TEST 1] GZIP Compression / Decompression:\n";
    std::string testStr = "Persona 5 Tactica Revolution Exporter Test String with GZIP!";
    std::vector<uint8_t> compressed;
    bool compOk = P5TCrypt::GzipCompress(reinterpret_cast<const uint8_t*>(testStr.data()), testStr.size(), compressed);
    assert(compOk);
    assert(compressed.size() > 10);
    assert(compressed[0] == 0x1f && compressed[1] == 0x8b && compressed[2] == 0x08); // GZIP magic
    std::cout << "  -> GZIP compression valid (" << compressed.size() << " bytes) [OK]\n";

    std::vector<uint8_t> decomp;
    bool decompOk = P5TCrypt::GzipDecompress(compressed.data(), compressed.size(), decomp);
    assert(decompOk);
    std::string decompStr(reinterpret_cast<const char*>(decomp.data()), decomp.size());
    assert(decompStr == testStr);
    std::cout << "  -> GZIP roundtrip verified [OK]\n";

    // [TEST 2] Inspecting Real PS4 Saves
    std::cout << "\n[TEST 2] Inspecting Real PS4 Saves:\n";
    std::vector<std::string> folders = {
        "dec_100_CUSA43148",
        "dec_200_CUSA43148",
        "dec_300_CUSA43148",
        "dec_301_CUSA43148",
        "dec_302_CUSA43148",
        "dec_303_CUSA43148",
        "dec_304_CUSA43148"
    };

    std::string baseDir = "Save/Persona 5 Tactica/PS4";
    for (const auto& f : folders) {
        std::string full = baseDir + "/" + f;
        if (fs::exists(full)) {
            P5TMetadata meta;
            std::string disp;
            bool ok = P5TCrypt::InspectSaveFile(full, meta, disp);
            assert(ok);
            std::cout << "  " << f << " -> " << disp << " | Temps: " << meta.playtimeFormatted << " [OK]\n";

            if (f == "dec_100_CUSA43148") {
                assert(meta.isSystem);
                assert(meta.dirName == "100");
            } else if (f == "dec_200_CUSA43148") {
                assert(meta.isAutoSave);
                assert(meta.dirName == "200");
            } else if (f == "dec_300_CUSA43148") {
                assert(meta.dirName == "300");
                assert(meta.protagonistName.find("Damien") != std::string::npos);
                assert(meta.teamLevel == 77);
            } else if (f == "dec_301_CUSA43148") {
                assert(meta.dirName == "301");
                assert(meta.protagonistName.find("Kokiwa") != std::string::npos);
                assert(meta.teamLevel == 22);
            }
        }
    }

    // [TEST 3] SaveInfo.dat Generation and Validation
    std::cout << "\n[TEST 3] SaveInfo.dat Generation and Validation:\n";
    P5TMetadata meta300;
    meta300.dirName = "300";
    meta300.slotNumber = 300;
    meta300.protagonistName = "Damien Garreau";
    meta300.teamLevel = 77;
    meta300.playtimeSeconds = 117624.6f;
    meta300.difficulty = 0;

    std::vector<uint8_t> gzipInfo;
    bool genOk = P5TCrypt::SerializeSaveInfoData(meta300, gzipInfo);
    assert(genOk);
    assert(gzipInfo.size() > 20);
    assert(gzipInfo[0] == 0x1f && gzipInfo[1] == 0x8b && gzipInfo[2] == 0x08);

    std::vector<uint8_t> plainInfo;
    bool infoDecomp = P5TCrypt::GzipDecompress(gzipInfo.data(), gzipInfo.size(), plainInfo);
    assert(infoDecomp);
    assert(plainInfo.size() >= sizeof(P5TCrypt::SAVEINFO_HEADER));
    assert(std::memcmp(plainInfo.data(), P5TCrypt::SAVEINFO_HEADER, sizeof(P5TCrypt::SAVEINFO_HEADER)) == 0);
    std::cout << "  -> SaveInfo.dat synthesized (" << gzipInfo.size() << " bytes GZIP) with valid .NET BinaryFormatter header [OK]\n";

    // [TEST 4] Full Conversion Pipeline
    std::cout << "\n[TEST 4] ConvertSaveFile Pipeline:\n";
    std::string testOutDir = "Save/test_p5t_pc";
    std::error_code ec;
    fs::remove_all(testOutDir, ec);

    std::string err;
    bool convOk = P5TCrypt::ConvertSaveFile("Save/Persona 5 Tactica/PS4/dec_300_CUSA43148", testOutDir, 300, err);
    if (!convOk) {
        std::cerr << "  Conversion failed: " << err << "\n";
    }
    assert(convOk);
    assert(fs::exists(testOutDir + "/300/SaveData.dat"));
    assert(fs::exists(testOutDir + "/300/SaveInfo.dat"));
    std::cout << "  -> Conversion to Save/test_p5t_pc/300/SaveData.dat + SaveInfo.dat successful [OK]\n";

    // Cleanup test output
    fs::remove_all(testOutDir, ec);

    std::cout << "\n========================================\n";
    std::cout << "  TOUS LES TESTS P5T ONT REUSSI !       \n";
    std::cout << "========================================\n";
    return 0;
}
