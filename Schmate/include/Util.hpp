
#pragma once
#include <cstdint>
#include <iostream>
#include <fstream>
#include <stdexcept>

inline void write_int64(std::ostream &os, int64_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    int64_t out = v;
#else
    int64_t out = __builtin_bswap64(v);
#endif
    os.write(reinterpret_cast<const char *>(&out), sizeof(out));
}

inline int64_t read_int64(std::istream &is) {
    int64_t in;
    is.read(reinterpret_cast<char *>(&in), sizeof(in));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return in;
#else
    return __builtin_bswap64(in);
#endif
}


