#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <variant>
#include "StringEmbedding.hpp"

namespace schmate_util { 

// Helper function to clean hex string
inline std::string cleanHexString(const std::string& hexStr) {
    std::string cleaned;
    for (char c : hexStr) {
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            cleaned += c;
        }
    }
    return cleaned;
}

// Helper function to parse a single hex character
inline uint8_t parseHexChar(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    throw std::invalid_argument("Invalid hex character");
}

// Convert hex string to binary values (0 or 1)
std::vector<int> hexToBinary(const std::string& hexStr) {
    std::string cleaned = cleanHexString(hexStr);
    std::vector<int> result;
    result.reserve(cleaned.length() * 4);
    
    for (char c : cleaned) {
        uint8_t val = parseHexChar(c);
        for (int i = 3; i >= 0; i--) {
            result.push_back((val >> i) & 1);
        }
    }
    
    return result;
}

// Convert hex string to int4 values (-8 to 7)
std::vector<int> hexToInt4(const std::string& hexStr) {
    std::string cleaned = cleanHexString(hexStr);
    std::vector<int> result;
    result.reserve(cleaned.length() * 2);
    
    for (char c : cleaned) {
        uint8_t val = parseHexChar(c);
        
        int8_t high = (val >> 4) & 0x0F;
        if (high & 0x08) high |= 0xF0;
        result.push_back(high);
        
        int8_t low = val & 0x0F;
        if (low & 0x08) low |= 0xF0;
        result.push_back(low);
    }
    
    return result;
}

// Convert hex string to int8 values (-128 to 127)
std::vector<int> hexToInt8(const std::string& hexStr) {
    std::string cleaned = cleanHexString(hexStr);
    
    if (cleaned.length() % 2 != 0) {
        throw std::invalid_argument("Hex string length must be even for int8");
    }
    
    std::vector<int> result;
    result.reserve(cleaned.length() / 2);
    
    for (size_t i = 0; i < cleaned.length(); i += 2) {
        uint8_t byte = (parseHexChar(cleaned[i]) << 4) | parseHexChar(cleaned[i + 1]);
        result.push_back(static_cast<int8_t>(byte));
    }
    
    return result;
}

// Convert hex string to float32 values
std::vector<float> hexToFloat32(const std::string& hexStr) {
    std::string cleaned = cleanHexString(hexStr);
    
    if (cleaned.length() % 8 != 0) {
        throw std::invalid_argument("Hex string length must be a multiple of 8 for float32");
    }
    
    std::vector<float> result;
    result.reserve(cleaned.length() / 8);
    
    for (size_t i = 0; i < cleaned.length(); i += 8) {
        uint32_t bits = 0;
        for (int j = 0; j < 8; j++) {
            bits = (bits << 4) | parseHexChar(cleaned[i + j]);
        }
        
        float value;
        std::memcpy(&value, &bits, sizeof(float));
        result.push_back(value);
    }
    
    return result;
}

// Determine data type based on dimension and hex string length
DataType inferDataType(const std::string& hexStr, size_t targetDimension) {
    std::string cleaned = cleanHexString(hexStr);
    size_t hexLength = cleaned.length();
    
    // Calculate bits available and bits needed per element
    size_t totalBits = hexLength * 4; // Each hex char = 4 bits
    
    // Check each type in order of preference
    if (totalBits == targetDimension * 32 && hexLength % 8 == 0) {
        return DataType::FLOAT32;
    }
    if (totalBits == targetDimension * 8 && hexLength % 2 == 0) {
        return DataType::INT8;
    }
    if (totalBits == targetDimension * 4) {
        return DataType::INT4;
    }
    if (totalBits == targetDimension) {
        return DataType::BINARY;
    }

    return DataType::TEXT;
}




} // namespace schmate_util
