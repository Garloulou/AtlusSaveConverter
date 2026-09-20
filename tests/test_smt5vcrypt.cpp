#include "core/SMT5VCrypt.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

#define CHECK(cond, msg) \
    if (!(cond)) { \
        std::cerr << "[FAIL] " << msg << " (line " << __LINE__ << ")" << std::endl; \
        return 1; \
    }

int main() {
    std::cout << "=== Running SMT5VCrypt Unit Tests ===" << std::endl;

    // 1. Test AES-256-ECB Roundtrip with arbitrary data
    {
        std::vector<uint8_t> plain(32, 0xAA);
        std::vector<uint8_t> enc;
        bool encOk = Ps4Atlus::SMT5VCrypt::EncryptAes256Ecb(plain.data(), plain.size(), enc);
        CHECK(encOk, "AES encrypt failed");
        CHECK(enc.size() == 32, "AES output size mismatch");
        CHECK(enc != plain, "AES output matches plaintext");

        std::vector<uint8_t> dec;
        bool decOk = Ps4Atlus::SMT5VCrypt::DecryptAes256Ecb(enc.data(), enc.size(), dec);
        CHECK(decOk, "AES decrypt failed");
        CHECK(dec == plain, "AES decrypted plaintext mismatch");
        std::cout << "[PASS] AES-256-ECB Roundtrip encryption/decryption passed." << std::endl;
    }

    // 2. Test SHA-1 Calculation
    {
        const char* msg = "Hello SMT5V";
        uint8_t hash[20] = {0};
        bool hashOk = Ps4Atlus::SMT5VCrypt::CalculateSha1(reinterpret_cast<const uint8_t*>(msg), strlen(msg), hash);
        CHECK(hashOk, "SHA-1 calculation failed");
        std::cout << "[PASS] SHA-1 engine computed digest successfully." << std::endl;
    }

    // 3. Test Save File Inspection & DLC cleaning on actual user file
    std::string testPath = "Save/Shin megami tensei V Vangeance/dec_GameSave01_CUSA42698/Megaten5.ps4.sav";
    if (fs::exists(testPath)) {
        int slot = 0;
        bool isSys = false;
        std::string dispName;
        Ps4Atlus::SMT5VCrypt::Smt5Metadata meta;

        bool ok = Ps4Atlus::SMT5VCrypt::InspectSaveFile(testPath, slot, isSys, dispName, &meta);
        CHECK(ok, "InspectSaveFile failed");
        CHECK(slot == 1, "Slot mismatch");
        CHECK(!isSys, "Unexpected isSys");
        CHECK(meta.protagonistName == "Damien", "Protagonist name mismatch");
        CHECK(meta.level == 48, "Level mismatch");
        CHECK(meta.dlcByte == 0x02, "DLC byte mismatch");
        CHECK(meta.isHashValid, "Hash invalid");
        std::cout << "[PASS] Megaten5.ps4.sav (Slot 1) inspected: " << dispName << std::endl;

        // Test encryption with DLC cleaning
        std::string outTestEnc = "tests/test_out_GameSave01.sav";
        Ps4Atlus::SMT5VCrypt::DlcCleanResult dlcRes;
        std::string err;
        bool encRes = Ps4Atlus::SMT5VCrypt::EncryptSaveFile(testPath, outTestEnc, true, dlcRes, err);
        CHECK(encRes, "EncryptSaveFile failed: " + err);
        CHECK(dlcRes.mainDlcCleared, "DLC flag not cleared");
        CHECK(dlcRes.originalDlcByte == 0x02, "Original DLC byte mismatch");
        std::cout << "[PASS] Save encrypted and DLC flag cleared to 0x00." << std::endl;

        // Decrypt the generated file to verify it can be read back and has valid SHA-1
        std::ifstream fIn(outTestEnc, std::ios::binary);
        std::vector<uint8_t> encFile((std::istreambuf_iterator<char>(fIn)), std::istreambuf_iterator<char>());
        fIn.close();

        std::vector<uint8_t> decFile;
        bool decFileOk = Ps4Atlus::SMT5VCrypt::DecryptAes256Ecb(encFile.data(), encFile.size(), decFile);
        CHECK(decFileOk, "DecryptAes256Ecb on output failed");
        CHECK(Ps4Atlus::SMT5VCrypt::IsDecrypted(decFile), "Output is not valid decrypted GVAS");
        CHECK(Ps4Atlus::SMT5VCrypt::ValidateSha1(decFile), "SHA-1 on cleaned output is invalid");
        CHECK(decFile[Ps4Atlus::SMT5VCrypt::OffsetDlcFlags] == 0x00, "DLC byte not zeroed in output");
        std::cout << "[PASS] Encrypted file verified: valid GVAS, valid SHA-1, DLC cleaned." << std::endl;

        // Cleanup test file
        std::error_code ec;
        fs::remove(outTestEnc, ec);
    } else {
        std::cout << "[WARN] Test file " << testPath << " not found, skipping disk check." << std::endl;
    }

    // 4. Test SysSave file
    std::string sysPath = "Save/Shin megami tensei V Vangeance/dec_SysSave_CUSA42698/Megaten5.ps4.sav";
    if (fs::exists(sysPath)) {
        int slot = 0;
        bool isSys = false;
        std::string dispName;
        Ps4Atlus::SMT5VCrypt::Smt5Metadata meta;

        bool ok = Ps4Atlus::SMT5VCrypt::InspectSaveFile(sysPath, slot, isSys, dispName, &meta);
        CHECK(ok, "SysSave inspection failed");
        CHECK(isSys, "SysSave not marked as system");
        CHECK(meta.isSystem, "meta.isSystem mismatch");
        CHECK(meta.isHashValid, "SysSave hash invalid");
        std::cout << "[PASS] dec_SysSave_CUSA42698 inspected: " << dispName << std::endl;

        std::string outSysEnc = "tests/test_out_SysSave.sav";
        Ps4Atlus::SMT5VCrypt::DlcCleanResult dlcRes;
        std::string err;
        bool encRes = Ps4Atlus::SMT5VCrypt::EncryptSaveFile(sysPath, outSysEnc, false, dlcRes, err);
        CHECK(encRes, "SysSave encryption failed: " + err);

        std::error_code ec;
        fs::remove(outSysEnc, ec);
        std::cout << "[PASS] SysSave encrypted successfully." << std::endl;
    }

    std::cout << "=== All SMT5VCrypt tests passed successfully! ===" << std::endl;
    return 0;
}
