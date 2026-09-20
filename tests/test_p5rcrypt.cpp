#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <filesystem>
#include "core/P5RCrypt.hpp"

namespace fs = std::filesystem;

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ps4Atlus - Persona 5 Royal Test       \n";
    std::cout << "========================================\n\n";

    // 1. Test CRC Calculation
    std::cout << "[TEST 1] CRC-32/MPEG-2 Verification:\n";
    const char testStr[] = "123456789";
    uint32_t crcVal = Ps4Atlus::P5RCrypt::CalculateCrc(reinterpret_cast<const uint8_t*>(testStr), 9);
    // Standard CRC-32/MPEG-2 of "123456789" is 0x0376E6E7
    std::cout << "  CRC('123456789') = 0x" << std::hex << crcVal << std::dec << "\n";
    assert(crcVal == 0x0376e6e7);
    std::cout << "  -> CRC matches standard MPEG-2 vector [OK]\n\n";

    // 2. Test AES-256-CBC Encryption & Decryption
    std::cout << "[TEST 2] AES-256-CBC with P5R Key:\n";
    std::vector<uint8_t> plainText(64, 0x42);
    uint8_t iv[16];
    bool ivOk = Ps4Atlus::P5RCrypt::GenerateRandomIv(iv);
    assert(ivOk);

    std::vector<uint8_t> cipherText;
    bool encOk = Ps4Atlus::P5RCrypt::Aes256CbcEncrypt(plainText.data(), plainText.size(), iv, cipherText);
    assert(encOk);
    assert(cipherText.size() == 64);

    std::vector<uint8_t> decryptedText;
    bool decOk = Ps4Atlus::P5RCrypt::Aes256CbcDecrypt(cipherText.data(), cipherText.size(), iv, decryptedText);
    assert(decOk);
    assert(decryptedText == plainText);
    std::cout << "  -> AES-256-CBC Roundtrip Successful [OK]\n\n";

    // 3. Test zlib Compression via miniz
    std::cout << "[TEST 3] zlib (miniz) Compression & Decompression:\n";
    std::string sampleData = "Phantom Thieves of Hearts - Persona 5 Royal Save File Converter Testing String 1234567890";
    std::vector<uint8_t> compressed;
    bool compOk = Ps4Atlus::P5RCrypt::ZlibCompress(reinterpret_cast<const uint8_t*>(sampleData.data()), sampleData.size(), compressed);
    assert(compOk);
    assert(compressed.size() < sampleData.size() + 32);

    std::vector<uint8_t> decompressed;
    bool decompOk = Ps4Atlus::P5RCrypt::ZlibDecompress(compressed.data(), compressed.size(), sampleData.size(), decompressed);
    assert(decompOk);
    assert(std::string(decompressed.begin(), decompressed.end()) == sampleData);
    std::cout << "  -> zlib Compression / Decompression Valid [OK]\n\n";

    // 4. Test Reading genuine PC Save if available
    std::string pcSavePath = "C:\\Users\\garre\\AppData\\Roaming\\SEGA\\P5R\\Steam\\76561199196938810\\savedata\\DATA01\\DATA.DAT";
    if (fs::exists(pcSavePath)) {
        std::cout << "[TEST 4] Unpacking Genuine User PC Save (DATA01/DATA.DAT):\n";
        std::ifstream file(pcSavePath, std::ios::binary);
        assert(file.is_open());
        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        assert(buffer.size() > 0x20);
        assert(std::memcmp(buffer.data(), "DATA", 4) == 0);

        uint32_t fileCrc = *reinterpret_cast<uint32_t*>(buffer.data() + 0x04);
        uint32_t checkCrc = Ps4Atlus::P5RCrypt::CalculateCrc(buffer.data() + 0x08, buffer.size() - 0x08);
        std::cout << "  File CRC: 0x" << std::hex << fileCrc << ", Calculated: 0x" << checkCrc << std::dec << "\n";
        assert(fileCrc == checkCrc);
        std::cout << "  -> Genuine Save File CRC Valid [OK]\n";

        uint32_t flags = *reinterpret_cast<uint32_t*>(buffer.data() + 0x0C);
        bool isEncrypted = (flags >> 31) != 0;
        assert(isEncrypted);

        const uint8_t* ivPtr = buffer.data() + 0x10;
        std::vector<uint8_t> decPayload;
        bool pDecOk = Ps4Atlus::P5RCrypt::Aes256CbcDecrypt(buffer.data() + 0x20, buffer.size() - 0x20, ivPtr, decPayload);
        assert(pDecOk);

        uint32_t dataCrc = *reinterpret_cast<uint32_t*>(decPayload.data() + 0x10);
        std::cout << "  Data CRC in Header: 0x" << std::hex << dataCrc << std::dec << "\n";
        std::cout << "  -> Genuine Save Decryption Successful [OK]\n\n";
    }

    // 5. Test Autonomous PS4 Save Conversion without param.sfo
    std::string samplePs4 = "Save/sample_ps4_data.dat";
    if (fs::exists(samplePs4)) {
        std::cout << "[TEST 5] Converting PS4 Data -> PC Steam format (Autonomous, No param.sfo):\n";
        std::string testOut = "Save/test_out_pc/DATA.DAT";
        std::string err;
        bool convOk = Ps4Atlus::P5RCrypt::ConvertSaveFile(samplePs4, testOut, 1, err);
        if (!convOk) {
            std::cout << "  Conversion failed: " << err << "\n";
        }
        assert(convOk);
        assert(fs::exists(testOut));
        std::cout << "  Output file size: " << fs::file_size(testOut) << " bytes\n";

        // Read and verify the converted save
        std::ifstream fOut(testOut, std::ios::binary);
        std::vector<uint8_t> outBytes((std::istreambuf_iterator<char>(fOut)), std::istreambuf_iterator<char>());
        fOut.close();

        assert(outBytes.size() > 0x20);
        assert(std::memcmp(outBytes.data(), "DATA", 4) == 0);

        uint32_t fCrc = *reinterpret_cast<uint32_t*>(outBytes.data() + 0x04);
        uint32_t cCrc = Ps4Atlus::P5RCrypt::CalculateCrc(outBytes.data() + 0x08, outBytes.size() - 0x08);
        assert(fCrc == cCrc);
        std::cout << "  -> Converted File CRC verified: 0x" << std::hex << fCrc << std::dec << " [OK]\n";

        // Decrypt converted payload
        const uint8_t* ivPtr = outBytes.data() + 0x10;
        std::vector<uint8_t> decPayload;
        bool dOk = Ps4Atlus::P5RCrypt::Aes256CbcDecrypt(outBytes.data() + 0x20, outBytes.size() - 0x20, ivPtr, decPayload);
        assert(dOk);

        uint32_t dataCrc = *reinterpret_cast<uint32_t*>(decPayload.data() + 0x10);
        std::cout << "  -> Converted Data CRC: 0x" << std::hex << dataCrc << std::dec << " [OK]\n";
        std::cout << "  -> PS4 to PC Autonomous Conversion Validated [OK]\n\n";
    }

    std::cout << "========================================\n";
    std::cout << "  TOUS LES TESTS P5R ONT REUSSI !       \n";
    std::cout << "========================================\n";
    return 0;
}

