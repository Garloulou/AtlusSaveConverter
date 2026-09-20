#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <algorithm>

namespace Ps4Atlus {

enum class P5SSaveFormat {
    Unknown,
    Switch_JP,
    Switch_EN,
    PC,
    PS4_JP,
    PS4_EN
};

struct P5SSaveFmtDesc {
    P5SSaveFormat format = P5SSaveFormat::Unknown;
    size_t size = 0;
    size_t nameLength = 33;
};

struct P5SSlotMetadata {
    int slotIndex = 0; // 0 to 9
    std::string firstName;
    std::string lastName;
    std::string fullName;
    uint32_t money = 0;
    uint16_t level = 1;
    bool hasData = false;
};

struct P5SMetadata {
    int activeSlot = -1; // -1 if none, 0..9
    std::vector<P5SSlotMetadata> slots;
    P5SSaveFormat detectedFormat = P5SSaveFormat::Unknown;
    bool isEncrypted = false;
};

class P5SCrypt {
public:
    static inline const P5SSaveFmtDesc FORMAT_SWITCH_JP = { P5SSaveFormat::Switch_JP, 0x600000, 13 };
    static inline const P5SSaveFmtDesc FORMAT_SWITCH_EN = { P5SSaveFormat::Switch_EN, 0x600000, 33 };
    static inline const P5SSaveFmtDesc FORMAT_PC        = { P5SSaveFormat::PC,        0x55DEA0, 33 };
    static inline const P5SSaveFmtDesc FORMAT_PS4_JP    = { P5SSaveFormat::PS4_JP,    0x55DB2C, 13 };
    static inline const P5SSaveFmtDesc FORMAT_PS4_EN    = { P5SSaveFormat::PS4_EN,    0x55DCBC, 33 };

    static const std::vector<P5SSaveFmtDesc>& GetAllFormats() {
        static const std::vector<P5SSaveFmtDesc> formats = {
            FORMAT_SWITCH_JP,
            FORMAT_SWITCH_EN,
            FORMAT_PC,
            FORMAT_PS4_JP,
            FORMAT_PS4_EN
        };
        return formats;
    }

    // Detect format from binary data and size
    static P5SSaveFmtDesc DetectFormat(const uint8_t* data, size_t size) {
        if (!data) return {};

        for (const auto& fmt : GetAllFormats()) {
            if (fmt.size == size) {
                size_t magicOffset = 0x1C + 0x88DF4 + fmt.nameLength * 2 + 0x938;
                if (magicOffset + 4 <= size) {
                    uint32_t magic = *reinterpret_cast<const uint32_t*>(data + magicOffset);
                    if (magic == 0x0036EE7F) {
                        return fmt;
                    }
                }
            }
        }

        // Fallback detection purely by exact file size
        for (const auto& fmt : GetAllFormats()) {
            if (fmt.size == size) {
                return fmt;
            }
        }

        return {};
    }

    // Check if a save buffer appears encrypted
    static bool IsEncrypted(const uint8_t* data, size_t size) {
        if (!data || size < 8) return false;
        int32_t slot = *reinterpret_cast<const int32_t*>(data + 4);
        return (slot < -1 || slot > 9);
    }

    // MSVC LCG XOR Crypt / Decrypt
    static void Crypt(uint8_t* data, size_t size, uint64_t steamId) {
        if (!data || size < 4) return;

        size_t cryptLen = size - 4;
        uint8_t checksum = 0;
        uint32_t state = static_cast<uint32_t>(steamId) ^ 0x20090501;

        if (cryptLen >= 2) {
            for (size_t i = 0; i < cryptLen; ++i) {
                state = 0x41C64E6D * state + 12345;
                checksum += data[i];
                data[i] ^= static_cast<uint8_t>(state >> 16);
            }
        }

        data[cryptLen] = checksum;
    }

    // Convert between save formats (e.g. PS4_EN -> PC, PS4_JP -> PC)
    static bool Convert(const uint8_t* src, size_t srcSize,
                        const P5SSaveFmtDesc& inFmt,
                        const P5SSaveFmtDesc& outFmt,
                        std::vector<uint8_t>& dst) {
        if (!src || srcSize < inFmt.size) return false;

        const size_t headerSize = 0x1C;
        const size_t baseSize = 0x88DF4;
        const size_t nameOffset = 0x87842;
        const size_t inSlotSize = baseSize + inFmt.nameLength * 2;
        const size_t outSlotSize = baseSize + outFmt.nameLength * 2;

        dst.assign(outFmt.size, 0);

        size_t srcPos = 0;
        size_t dstPos = 0;

        // 1. Copy Header (0x1C bytes)
        std::memcpy(dst.data(), src, headerSize);
        srcPos += headerSize;
        dstPos += headerSize;

        // 2. Copy 10 slots
        for (int i = 0; i < 10; ++i) {
            size_t slotSrcStart = headerSize + i * inSlotSize;
            size_t slotDstStart = headerSize + i * outSlotSize;

            if (slotSrcStart + inSlotSize > srcSize || slotDstStart + outSlotSize > dst.size()) {
                break;
            }

            // Copy data upto fname
            std::memcpy(dst.data() + slotDstStart, src + slotSrcStart, nameOffset);

            // Copy fname (null padded)
            const uint8_t* fnameSrc = src + slotSrcStart + nameOffset;
            uint8_t* fnameDst = dst.data() + slotDstStart + nameOffset;
            size_t copyFnameLen = (std::min)(inFmt.nameLength, outFmt.nameLength);
            if (copyFnameLen > 0) {
                std::memcpy(fnameDst, fnameSrc, copyFnameLen - 1);
                fnameDst[outFmt.nameLength - 1] = 0;
            }

            // Copy lname (null padded)
            const uint8_t* lnameSrc = fnameSrc + inFmt.nameLength;
            uint8_t* lnameDst = fnameDst + outFmt.nameLength;
            size_t copyLnameLen = (std::min)(inFmt.nameLength, outFmt.nameLength);
            if (copyLnameLen > 0) {
                std::memcpy(lnameDst, lnameSrc, copyLnameLen - 1);
                lnameDst[outFmt.nameLength - 1] = 0;
            }

            // Copy rest of slot
            size_t restOffsetSrc = slotSrcStart + nameOffset + inFmt.nameLength * 2;
            size_t restOffsetDst = slotDstStart + nameOffset + outFmt.nameLength * 2;
            size_t restLen = baseSize - nameOffset;
            std::memcpy(dst.data() + restOffsetDst, src + restOffsetSrc, restLen);
        }

        // 3. Copy trailing system/configuration data
        size_t trailingSrcStart = headerSize + 10 * inSlotSize;
        size_t trailingDstStart = headerSize + 10 * outSlotSize;
        if (trailingSrcStart < srcSize && trailingDstStart < dst.size()) {
            size_t trailSrcLen = srcSize - trailingSrcStart;
            size_t trailDstLen = dst.size() - trailingDstStart;
            size_t copyTrail = (std::min)(trailSrcLen, trailDstLen);
            std::memcpy(dst.data() + trailingDstStart, src + trailingSrcStart, copyTrail);
        }

        return true;
    }

    // Inspect P5S save container
    static bool InspectSaveFile(const std::string& filePath,
                                uint64_t steamId,
                                P5SMetadata& outMeta,
                                std::string& outDisplayName) {
        std::error_code ec;
        if (!std::filesystem::exists(filePath, ec)) return false;

        std::ifstream in(filePath, std::ios::binary);
        if (!in.is_open()) return false;
        in.seekg(0, std::ios::end);
        size_t fsize = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(fsize);
        in.read(reinterpret_cast<char*>(buffer.data()), fsize);
        in.close();

        if (buffer.size() < 0x20) return false;

        bool encrypted = IsEncrypted(buffer.data(), buffer.size());
        outMeta.isEncrypted = encrypted;

        if (encrypted && steamId != 0) {
            Crypt(buffer.data(), buffer.size(), steamId);
            if (IsEncrypted(buffer.data(), buffer.size())) {
                // Decryption failed
                outDisplayName = "SAVEDATA.BIN (Chiffré - Clé incorrecte)";
                return true;
            }
        }

        P5SSaveFmtDesc fmt = DetectFormat(buffer.data(), buffer.size());
        outMeta.detectedFormat = fmt.format;

        int32_t active = *reinterpret_cast<const int32_t*>(buffer.data() + 4);
        outMeta.activeSlot = active;

        size_t nameLen = fmt.nameLength > 0 ? fmt.nameLength : 33;
        size_t slotSize = 0x88DF4 + nameLen * 2;
        size_t headerSize = 0x1C;

        outMeta.slots.clear();
        for (int i = 0; i < 10; ++i) {
            size_t slotOffset = headerSize + i * slotSize;
            if (slotOffset + slotSize > buffer.size()) break;

            P5SSlotMetadata slotMeta;
            slotMeta.slotIndex = i;

            size_t nameOffset = slotOffset + 0x87842;
            std::string fname(reinterpret_cast<const char*>(buffer.data() + nameOffset), nameLen);
            size_t fNull = fname.find('\0');
            if (fNull != std::string::npos) fname.resize(fNull);

            std::string lname(reinterpret_cast<const char*>(buffer.data() + nameOffset + nameLen), nameLen);
            size_t lNull = lname.find('\0');
            if (lNull != std::string::npos) lname.resize(lNull);

            slotMeta.firstName = fname;
            slotMeta.lastName = lname;
            slotMeta.fullName = (fname.empty() && lname.empty()) ? "" : (fname + " " + lname);

            slotMeta.money = *reinterpret_cast<const uint32_t*>(buffer.data() + slotOffset);
            slotMeta.level = *reinterpret_cast<const uint16_t*>(buffer.data() + slotOffset + 4);

            slotMeta.hasData = (!slotMeta.fullName.empty() || slotMeta.level > 1 || slotMeta.money > 0);

            outMeta.slots.push_back(slotMeta);
        }

        outDisplayName = "SAVEDATA.BIN (10 Emplacements)";
        return true;
    }

    // Convert PS4 decrypted save to PC format with automatic Steam ID encryption
    static bool ConvertSaveFile(const std::string& inputPath,
                                const std::string& outputPath,
                                uint64_t steamId,
                                std::string& error) {
        std::error_code ec;
        if (!std::filesystem::exists(inputPath, ec)) {
            error = "Fichier source introuvable : " + inputPath;
            return false;
        }

        std::ifstream in(inputPath, std::ios::binary);
        if (!in.is_open()) {
            error = "Impossible d'ouvrir le fichier source : " + inputPath;
            return false;
        }

        in.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);

        std::vector<uint8_t> srcData(fileSize);
        in.read(reinterpret_cast<char*>(srcData.data()), fileSize);
        in.close();

        // If source is already encrypted PC save, decrypt it first
        if (IsEncrypted(srcData.data(), srcData.size())) {
            Crypt(srcData.data(), srcData.size(), steamId);
        }

        P5SSaveFmtDesc inFmt = DetectFormat(srcData.data(), srcData.size());
        if (inFmt.format == P5SSaveFormat::Unknown) {
            // Default to PS4_EN if size matches or close
            if (fileSize == FORMAT_PS4_EN.size) inFmt = FORMAT_PS4_EN;
            else if (fileSize == FORMAT_PS4_JP.size) inFmt = FORMAT_PS4_JP;
            else if (fileSize == FORMAT_PC.size) inFmt = FORMAT_PC;
            else {
                error = "Format de sauvegarde Persona 5 Strikers inconnu ou non supporté.";
                return false;
            }
        }

        std::vector<uint8_t> pcData;
        if (inFmt.format == P5SSaveFormat::PC) {
            pcData = srcData;
        } else {
            if (!Convert(srcData.data(), srcData.size(), inFmt, FORMAT_PC, pcData)) {
                error = "Échec de la conversion de structure de la sauvegarde vers le format PC.";
                return false;
            }
        }

        // Encrypt with destination Steam ID
        Crypt(pcData.data(), pcData.size(), steamId);

        // Ensure output directory exists
        std::filesystem::path outPath(outputPath);
        std::filesystem::create_directories(outPath.parent_path(), ec);

        std::ofstream out(outputPath, std::ios::binary);
        if (!out.is_open()) {
            error = "Impossible de créer le fichier cible : " + outputPath;
            return false;
        }

        out.write(reinterpret_cast<const char*>(pcData.data()), pcData.size());
        out.close();

        return true;
    }
};

} // namespace Ps4Atlus
