#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <string>
#include <vector>

// Minimal PNG writer (8-bit RGB, uncompressed deflate blocks).
namespace png {

inline const uint32_t* crc_table() {
    static uint32_t table[256];
    static bool built = false;
    if (!built) {
        for (uint32_t n = 0; n < 256; n++) {
            uint32_t c = n;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : (c >> 1);
            table[n] = c;
        }
        built = true;
    }
    return table;
}

inline uint32_t crc32_buf(const uint8_t* buf, size_t len, uint32_t crc = 0xFFFFFFFFu) {
    const uint32_t* t = crc_table();
    for (size_t i = 0; i < len; i++) crc = t[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

inline void put_u32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(uint8_t(v >> 24));
    out.push_back(uint8_t(v >> 16));
    out.push_back(uint8_t(v >> 8));
    out.push_back(uint8_t(v));
}

inline void write_chunk(std::vector<uint8_t>& out, const char type[5],
                        const std::vector<uint8_t>& data) {
    put_u32(out, uint32_t(data.size()));
    size_t crc_start = out.size();
    for (int i = 0; i < 4; i++) out.push_back(uint8_t(type[i]));
    out.insert(out.end(), data.begin(), data.end());
    uint32_t crc = crc32_buf(out.data() + crc_start, out.size() - crc_start) ^ 0xFFFFFFFFu;
    put_u32(out, crc);
}

// `pixels` is width*height*3 bytes, row-major, RGB.
inline bool write_rgb(const std::string& path, int width, int height,
                      const std::vector<uint8_t>& pixels) {
    // Raw scanlines, each prefixed with filter type 0 (None).
    std::vector<uint8_t> raw;
    raw.reserve(size_t(height) * (size_t(width) * 3 + 1));
    for (int y = 0; y < height; y++) {
        raw.push_back(0);
        const uint8_t* row = pixels.data() + size_t(y) * size_t(width) * 3;
        raw.insert(raw.end(), row, row + size_t(width) * 3);
    }

    // zlib wrapper + stored deflate blocks.
    std::vector<uint8_t> z;
    z.push_back(0x78);
    z.push_back(0x01);
    const size_t kBlock = 65535;
    for (size_t off = 0; off < raw.size() || raw.empty(); off += kBlock) {
        size_t len = std::min(kBlock, raw.size() - off);
        bool final_block = (off + len >= raw.size());
        z.push_back(final_block ? 1 : 0);
        z.push_back(uint8_t(len & 0xFF));
        z.push_back(uint8_t((len >> 8) & 0xFF));
        z.push_back(uint8_t((~len) & 0xFF));
        z.push_back(uint8_t(((~len) >> 8) & 0xFF));
        z.insert(z.end(), raw.begin() + off, raw.begin() + off + len);
        if (final_block) break;
    }
    // adler32 of the raw data
    uint32_t a = 1, b = 0;
    for (uint8_t byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    put_u32(z, (b << 16) | a);

    std::vector<uint8_t> out;
    const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    out.insert(out.end(), sig, sig + 8);

    std::vector<uint8_t> ihdr;
    put_u32(ihdr, uint32_t(width));
    put_u32(ihdr, uint32_t(height));
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(2);  // color type: truecolor RGB
    ihdr.push_back(0);  // deflate
    ihdr.push_back(0);  // adaptive filtering
    ihdr.push_back(0);  // no interlace
    write_chunk(out, "IHDR", ihdr);
    write_chunk(out, "IDAT", z);
    write_chunk(out, "IEND", {});

    // Create the output directory if needed.
    std::error_code ec;
    std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, ec);

    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    size_t written = std::fwrite(out.data(), 1, out.size(), f);
    std::fclose(f);
    return written == out.size();
}

}  // namespace png
