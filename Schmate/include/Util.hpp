
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

#include <filesystem>


// Search for a filename in a UNIX style path (dir1:dir2) and confirm it's .ggml
// If found returns the now qualified path else just the filename
std::string find_ggml_model(const std::string &filename, const std::string  &search_paths);


// File exist and length utils
inline bool file_exists(const std::string &p) {
      return std::filesystem::exists(p);
}
// Does not exist? Return -1 else return its size
inline off_t file_size(const std::string &p) {
    if (!file_exists(p)) return -1;
    return std::filesystem::file_size(p);
}

#pragma once
#ifdef __APPLE__
void relax_macos_malloc_zones();
#else
inline void relax_macos_malloc_zones() {}
#endif
