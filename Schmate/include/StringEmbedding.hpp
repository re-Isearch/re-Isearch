#pragma once

// Utils to allow for code to pass hex encoded vectors to by-pass
// the Sbert layer. 

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <variant>

namespace schmate_util { 

enum class DataType {
    TEXT,     // Text, not encoded 
    BINARY,   // 1 bit per value (0 or 1)
    INT4,     // 4 bits per value (-8 to 7)
    INT8,     // 8 bits per value (-128 to 127)
    FLOAT32   // 32 bits per value
};

// Convert hex string to binary values (0 or 1)
std::vector<int> hexToBinary(const std::string& hexStr);


// Convert hex string to int4 values (-8 to 7)
std::vector<int> hexToInt4(const std::string& hexStr);


// Convert hex string to int8 values (-128 to 127)
std::vector<int> hexToInt8(const std::string& hexStr);

// Convert hex string to float32 values
std::vector<float> hexToFloat32(const std::string& hexStr);


// Determine data type based on dimension and hex string length
DataType inferDataType(const std::string& hexStr, size_t targetDimension);


// Automatic conversion based on dimension
// Returns std::variant that can hold any of the vector types
using VectorVariant = std::variant<
    std::vector<int>,    // For binary, int4, int8
    std::vector<float>   // For float32
>;

inline VectorVariant hexToVector(const std::string& hexStr, size_t targetDimension) {
    DataType type = inferDataType(hexStr, targetDimension);
    
    switch (type) {
        case DataType::BINARY:
            return hexToBinary(hexStr);
        case DataType::INT4:
            return hexToInt4(hexStr);
        case DataType::INT8:
            return hexToInt8(hexStr);
        case DataType::FLOAT32:
            return hexToFloat32(hexStr);
        default:
            throw std::invalid_argument("Unknown data type");
    }
}

// Overloaded versions that return specific types
inline std::vector<int> hexToVectorInt(const std::string& hexStr, size_t targetDimension) {
    DataType type = inferDataType(hexStr, targetDimension);
    
    switch (type) {
        case DataType::BINARY:
            return hexToBinary(hexStr);
        case DataType::INT4:
            return hexToInt4(hexStr);
        case DataType::INT8:
            return hexToInt8(hexStr);
        default:
            throw std::invalid_argument("Data type is not integer-based");
    }
}

inline std::vector<float> hexToVectorFloat(const std::string& hexStr, size_t targetDimension) {
    DataType type = inferDataType(hexStr, targetDimension);
    
    if (type != DataType::FLOAT32) {
        throw std::invalid_argument("Data type is not float32");
    }
    
    return hexToFloat32(hexStr);
}

// Fast validation: check if string is hex-encoded float32 vector of given dimension
inline bool isHexFloat32Vector(const std::string_view str, size_t targetDimension) {
    // Fast length check first
    size_t expectedLength = targetDimension * 8; // 8 hex chars per float32

    size_t hexCount = 0;
    size_t consecutiveHex = 0;

    // Single pass: count hex chars and validate spacing
    for (char c : str) {
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            hexCount++;
            consecutiveHex++;
            // Early exit if too long
            if (hexCount > expectedLength) {
                return false;
            }
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            // Whitespace is only allowed at even positions (after complete byte pairs)
            if (consecutiveHex & 1 /*consecutiveHex % 2 != 0 */) {
                return false;
            }
            consecutiveHex = 0;
        } else {
            // Invalid character (not hex, not whitespace)
            return false;
        }
    }

    // Exact match required and final position must be even
    return hexCount == expectedLength && consecutiveHex & 1 /* consecutiveHex % 2 == 0 */;
}

// Fast validation: check if string is hex-encoded float32 vector of given dimension
//inline bool isHexFloat32Vector(const std::string& str, size_t targetDimension) {
//   return HexFloat32VectorLength(str) == targetDimension * 8;
//}


} // namespace schmate_util
