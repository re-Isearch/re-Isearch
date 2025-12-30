#pragma once
#include <cstdint>

namespace hfmap {

constexpr uint32_t kMagic   = 0x314D4648; // 'HFM1'
constexpr uint32_t kVersion = 1;
constexpr uint32_t kEmpty   = 0xFFFFFFFF;

#pragma pack(push, 1)

struct Header {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_count;
    uint32_t table_size;   // power of two
    uint32_t pool_size;    // bytes
};

struct Entry {
    uint32_t key_offset;   // into pool
    uint32_t val_offset;
    uint32_t hash;
    uint32_t reserved;
};

#pragma pack(pop)


// Fowler–Noll–Vo v.1a non-cryptographic hash function 
// http://www.isthe.com/chongo/tech/comp/fnv/index.html#xor-fold
inline uint32_t fnv1a(std::string_view s) {
    uint32_t h = 2166136261u; 
    for (unsigned char c : s)
       h = (h ^ c) * 16777619u;
    return h;
}       

static_assert(sizeof(Header) == 20, "Header size mismatch");
static_assert(sizeof(Entry)  == 16, "Entry size mismatch");

} // namespace hfmap

