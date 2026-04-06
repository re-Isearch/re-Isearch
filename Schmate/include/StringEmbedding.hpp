#pragma once

// Utils to allow for code to pass hex encoded vectors to by-pass
// the Sbert layer. 

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <variant>


/*

The ability to store hex-encoded string represenations of vectors in documents provide a number of key benefits:

Model agnostic (but with a hard consistency contraint)
   Enables the use of other vectorization pipelines. All that matters is that they are consistent.
   Any model that can produce vectors of the right dimension and encode them as hex strings can feed into it:
   - A different SBERT model
   - OpenAI embeddings
   - Cohere, Mistral, or any other embedding API
   - A custom fine-tuned model
   - A completely different architecture like a CNN or autoencoder
   The DB just stores and retrieves hex strings. The search engine just computes distances and does not care
   where the numbers came from.

   The consistency constraint is the one hard rule — you can't mix embeddings from different models in the
   same index because:
   - The vector spaces are incompatible — cosine similarity between a SBERT vector and an OpenAI vector is meaningless
   - The dimensions may differ
   - The magnitude distributions differ, breaking nearest-neighbour assumptions

Storage agnosticism
   The vector data is just a string as far as the database is concerned. No special binary column types,
   no BLOB handling, no endianness issues. It can be stored, retrieved, replicated, and indexed by any
   system that understands strings — including our own ingest.

   NOTE: While the hex encoding is model-agnostic at the storage layer, the index layer must be model-homogeneous. 
 
Human readable / debuggable:
   You can look at a raw document and see the hex string. You can diff two revisions. You can spot corruption.
   Binary BLOBs are opaque; hex strings are not.

Self describing via inferDataType:
   The hex string encodes enough information that inferDataType can reconstruct what it is — BINARY, INT4,
   INT8, or FLOAT32 — just from the string and the target dimension.
   The encoding is the type descriptor. No separate metadata column needed.

Portability across languages:
   Any language can encode and decode hex strings. A Python indexer, a C++ search engine, a JavaScript UI — they
   all interoperate trivially without agreeing on binary serialisation formats, struct packing, or endianness.

Compact but lossless:
   Hex is more compact than base10 text representations of floats, and unlike base64 it is trivially human-readable.
   For quantised vectors (BINARY, INT4, INT8) the savings over storing raw float32 are substantial — a BINARY
   vector is 32x smaller than its float32 equivalent.

The round-trip guarantee:
   The functions vectorToHex / hexToVector pair ensures what goes in comes out identically — the hex string is a
   stable, canonical representation that survives storage, retrieval, replication, and re-indexing without drift.

   auto [values, type] = hexToVectorWithType(hexStr, targetDimension);
   std::string hexBack = vectorToHex(values, type);

*/

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

// Functions to encode vectors into Hex strings
std::string binaryToHex(const std::vector<int>& values);
std::string int4ToHex(const std::vector<int>& values);
std::string int8ToHex(const std::vector<int>& values);
std::string float32ToHex(const std::vector<float>& values);


inline std::string vectorToHex(const std::vector<int>& values, DataType type) {
    switch (type) {
        case DataType::BINARY:  return binaryToHex(values);
        case DataType::INT4:    return int4ToHex(values);
        case DataType::INT8:    return int8ToHex(values);
        default: throw std::runtime_error("Invalid DataType for int vector");
    }
}

inline std::string vectorToHex(const std::vector<float>& values, DataType type = DataType::FLOAT32) {
    if (type != DataType::FLOAT32)
        throw std::runtime_error("Invalid DataType for float vector");
    return float32ToHex(values);
}

inline std::string vectorToHex(const VectorVariant& values, DataType type) {
    return std::visit([type](const auto& vec) -> std::string {
        using T = std::decay_t<decltype(vec)>;
        if constexpr (std::is_same_v<T, std::vector<float>>) {
            return float32ToHex(vec);
        } else {
            switch (type) {
                case DataType::BINARY: return binaryToHex(vec);
                case DataType::INT4:   return int4ToHex(vec);
                case DataType::INT8:   return int8ToHex(vec);
                default: throw std::invalid_argument("Data type is not integer-based");
            }
        }
    }, values);
}


template<typename T>
inline std::vector<T> hexToVector(const std::string& hex, DataType type) = delete;

template<>
inline std::vector<int> hexToVector<int>(const std::string& hex, DataType type) {
    switch (type) {
        case DataType::BINARY: return hexToBinary(hex);
        case DataType::INT4:   return hexToInt4(hex);
        case DataType::INT8:   return hexToInt8(hex);
        default: throw std::invalid_argument("DataType is not integer-based");
    }
}

template<>
inline std::vector<float> hexToVector<float>(const std::string& hex, DataType type) {
    if (type != DataType::FLOAT32)
        throw std::invalid_argument("DataType is not float-based");
    return hexToFloat32(hex);
}


template<typename T>
inline std::pair<std::vector<T>, DataType> hexToVecWithType(const std::string& hex, size_t targetDimension) = delete;

template<>
inline std::pair<std::vector<int>, DataType> hexToVecWithType<int>(const std::string& hex, size_t targetDimension) {
    DataType type = inferDataType(hex, targetDimension);
    return { hexToVector<int>(hex, type), type };
}

template<>
inline std::pair<std::vector<float>, DataType> hexToVecWithType<float>(const std::string& hex, size_t targetDimension) {
    DataType type = inferDataType(hex, targetDimension);
    return { hexToVector<float>(hex, type), type };
}

} // namespace schmate_util
