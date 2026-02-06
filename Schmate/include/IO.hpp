#pragma once
#include <cstdint>
#include <iostream>
#include <fstream>
#include <stdexcept>


// ========= Read and Write ===================================== //
template<typename T>
inline T read_le(std::ifstream &f) {
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    
    T value;
    f.read(reinterpret_cast<char*>(&value), sizeof(T));
    
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    if constexpr (sizeof(T) == 2) {
        value = __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        value = __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        value = __builtin_bswap64(value);
    }
    #endif
    
    return value;
}

template<typename T>
inline std::optional<T> read_le_safe(std::ifstream &f) {
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    
    T value;
    if (!f.read(reinterpret_cast<char*>(&value), sizeof(T))) {
        return std::nullopt;  // Read failed
    }
    
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    if constexpr (sizeof(T) == 2) {
        value = __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        value = __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        value = __builtin_bswap64(value);
    }
    #endif
    
    return value;
}

template<typename T>
inline void write_le(std::ofstream &f, T value) {
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    if constexpr (sizeof(T) == 2) {
        value = __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        value = __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        value = __builtin_bswap64(value);
    }
    #endif
    
    f.write(reinterpret_cast<const char*>(&value), sizeof(T));
}


inline void write_int64(std::ofstream &os, int64_t v) {
    write_le<int64_t>(os, v);
}

inline int64_t read_int64(std::ifstream &is) {
    return read_le<int64_t>(is);
}


// This function intentionally does not write strings longer than 65535 bytes
// as the expected string should probably never be longer than 512 bytes.
inline size_t write_string(std::ofstream& os , const std::string& string) {
    size_t len = string.length();
    // We don't ever want longer-- that should never happen here
    if (len > UINT16_MAX) len = 0;
    write_le<uint16_t>(os, (uint16_t) len); // Write the length of the string
    if (len) os.write(string.c_str(), len); // even if string contains \0 its OK!
    return len;
}

inline const std::string read_string(std::ifstream &is) {
   uint16_t bytes = read_le<uint16_t>(is);
   if (bytes == 0) return "";

   std::string str (bytes, '\0');
   is.read(str.data(), bytes);
   return str;
}

