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

///////////////////////////////////////////////////
///// Base64 Support
///////////////////////////////////////////////////


// Determine data type from base64 string length and target dimension
// Mirrors inferDataType for hex but accounts for base64 overhead.
// MongoDB BSON vector subtype 0x09 prepends 2 bytes (dtype + padding),
// so we check both with and without the 2-byte header.
DataType inferBase64DataType(const std::string& b64Str, size_t targetDimension) ;

std::vector<uint8_t> base64Decode(const std::string& b64Str);

// Fast validation: check if string is base64-encoded float32 vector of given dimension
// Each float32 = 4 bytes. Base64 encodes 3 bytes → 4 chars, padded to multiple of 4.
// So dim float32s = dim*4 bytes → ceil(dim*4/3)*4 chars (with = padding)
inline bool isBase64Float32Vector(const std::string_view str, size_t targetDimension)
{
  size_t byteCount    = targetDimension * 4;          // bytes needed
  size_t expectedLen  = ((byteCount + 2) / 3) * 4;   // base64 chars with padding

  size_t b64Count   = 0;
  size_t padCount   = 0;
  bool   seenPad    = false;

  for (char c : str)
    {
      if (seenPad)
        {
          // After first '=' only another '=' is legal (at most 2 padding chars)
          if (c == '=') { padCount++; if (padCount > 2) return false; }
          else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
          else return false;
        }

      if ((c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') ||
          c == '+' || c == '/')
        {
          b64Count++;
          if (b64Count > expectedLen) return false; // too long
        }
      else if (c == '=')
        {
          seenPad  = true;
          padCount = 1;
        }
      else if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
          // Whitespace only valid on 4-char boundaries
          if (b64Count % 4 != 0) return false;
        }
      else
        return false; // invalid character
    }

  return (b64Count + padCount) == expectedLen &&
         (b64Count + padCount) % 4 == 0;
}


inline bool isBase64UrlFloat32Vector(const std::string_view str, size_t targetDimension)
{
  // base64url: '+' -> '-', '/' -> '_', padding optional
  size_t byteCount   = targetDimension * 4;
  size_t expectedLen = ((byteCount + 2) / 3) * 4;
  size_t b64Count    = 0;

  for (char c : str)
    {
      if ((c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') ||
          c == '-' || c == '_')
        {
          b64Count++;
          if (b64Count > expectedLen) return false;
        }
      else if (c == '=') continue;  // padding optional, ignore
      else return false;
    }

  // Without padding: encoded length rounds up to multiple of 4
  return ((b64Count + 2) / 3) * 4 == expectedLen;
}

// Expected base64 encoded length for dim values of given type
// base64: 3 bytes → 4 chars, padded to multiple of 4
inline size_t base64ExpectedLength(size_t byteCount)
{
  return ((byteCount + 2) / 3) * 4;
}


std::vector<float> base64ToFloat32(const std::string& b64Str);
std::vector<int> base64ToInt8(const std::string& b64Str);
std::vector<int> base64ToInt4(const std::string& b64Str);
std::vector<int> base64ToBinary(const std::string& b64Str);


// Automatic conversion — mirrors hexToVector interface exactly
inline VectorVariant base64ToVector(const std::string& b64Str, size_t targetDimension)
{
  DataType type = inferBase64DataType(b64Str, targetDimension);
  switch (type)
    {
      case DataType::BINARY:  return base64ToBinary(b64Str);
      case DataType::INT4:    return base64ToInt4(b64Str);
      case DataType::INT8:    return base64ToInt8(b64Str);
      case DataType::FLOAT32: return base64ToFloat32(b64Str);
      default:
        throw std::invalid_argument("Unknown data type");
    }
}

// Unified entry point: auto-detect hex vs base64 and dispatch
inline VectorVariant decodeVector(const std::string& str, size_t targetDimension)
{
  // Hex: only [0-9a-fA-F], length = dim * bytes_per_value * 2
  // Base64: contains [A-Z a-z 0-9 + / = -_], shorter than hex for same data
  // Quick discriminator: if all chars are hex digits → try hex first
  bool couldBeHex = true;
  for (char c : str)
    if (!isxdigit((unsigned char)c) && c != ' ' && c != '\n' && c != '\r' && c != '\t')
      { couldBeHex = false; break; }

  if (couldBeHex)
    return hexToVector(str, targetDimension);
  else
    return base64ToVector(str, targetDimension);
}


inline bool isEncodedFloat32Vector(const std::string_view str, size_t targetDimension)
{
  return isHexFloat32Vector(str, targetDimension) ||
	isBase64Float32Vector(str, targetDimension) ;
}


/// MAIN DECODE ROUTINES

// Pad any quantized vector to float32 representation.
// INT4 (-8..7), INT8 (-128..127), BINARY (0/1) all convert losslessly.
// float32 input is a no-op (returned as-is).
inline std::vector<float> toFloat32(const VectorVariant& v, DataType sourceType)
{
  return std::visit([sourceType](const auto& vec) -> std::vector<float>
    {
      if constexpr (std::is_same_v<std::decay_t<decltype(vec)>, std::vector<float>>)
        {
          // Already float32 — no-op
          return vec;
        }
      else
        {
          std::vector<float> out;
          out.reserve(vec.size());
          switch (sourceType)
            {
              case DataType::BINARY:
                // 0 → 0.0f, 1 → 1.0f
                for (int v : vec) out.push_back(v ? 1.0f : 0.0f);
                break;

              case DataType::INT4:
                // -8..7 → normalise to [-1, 1] range: v / 8.0f
                // Keeps the geometric relationships intact for ANN
                for (int v : vec) out.push_back(v / 8.0f);
                break;

              case DataType::INT8:
                // -128..127 → normalise to [-1, 1] range: v / 128.0f
                for (int v : vec) out.push_back(v / 128.0f);
                break;

              default:
                // Unknown quantization — cast as-is
                for (int v : vec) out.push_back(static_cast<float>(v));
                break;
            }
          return out;
        }
    }, v);
}

// Convenience: decode any hex or base64 vector string and return float32.
// This is the single entry point for the append pipeline — caller never
// needs to know what quantization was used in the source data.
inline std::vector<float> decodeToFloat32(const std::string& str,
                                           size_t targetDimension)
{
  DataType  type;
  VectorVariant v;

  // Auto-detect encoding and quantization
  bool couldBeHex = true;
  for (char c : str)
    if (!isxdigit((unsigned char)c) &&
        c != ' ' && c != '\n' && c != '\r' && c != '\t')
      { couldBeHex = false; break; }

  if (couldBeHex)
    {
      type = inferDataType(str, targetDimension);
      v    = hexToVector(str, targetDimension);
    }
  else
    {
      type = inferBase64DataType(str, targetDimension);
      v    = base64ToVector(str, targetDimension);
    }

  return toFloat32(v, type);
}

// A quick gate before the more expensive inferDataType:
inline bool couldBeEncodedVector(const std::string_view str, size_t targetDimension)
{
  // Check if length matches any quantization level in either encoding
  size_t len = str.size();
  // Hex lengths: dim/8*2, dim/2*2, dim*2, dim*8  (binary,int4,int8,float32)
  if (len == targetDimension/4   || // binary hex
      len == targetDimension      || // int4 hex
      len == targetDimension*2    || // int8 hex
      len == targetDimension*8)      // float32 hex (already caught above)
    return true;
  // Base64 lengths: approximately 4/3 of byte count
  size_t b = (len * 3) / 4;  // approximate byte count
  if (b == targetDimension/8  ||  // binary
      b == targetDimension/2  ||  // int4
      b == targetDimension    ||  // int8
      b == targetDimension*4)     // float32 (already caught above)
    return true;
  return false;
}

//// Raw Vectors

// Fast validation: check if string is a JSON-style float array of given dimension
// Accepts: "[0.123, -0.456, 0.789]" or "0.123, -0.456, 0.789" (with or without brackets)
// Handles: scientific notation (1.23e-4), integers (0, 1), signed values (-0.5)
inline bool isFloatArrayVector(const std::string_view str, size_t targetDimension)
{
  if (str.empty()) return false;

  size_t   pos   = 0;
  size_t   count = 0;
  bool     hasBracket = false;
  size_t   len   = str.size();

  // Skip leading whitespace
  while (pos < len && isspace((unsigned char)str[pos])) ++pos;

  // Optional opening bracket
  if (pos < len && str[pos] == '[') { hasBracket = true; ++pos; }

  while (pos < len)
    {
      // Skip whitespace and commas
      while (pos < len && (isspace((unsigned char)str[pos]) ||
                           str[pos] == ',')) ++pos;

      // Closing bracket
      if (pos < len && str[pos] == ']')
        {
          if (!hasBracket) return false;  // unexpected ']'
          ++pos;
          // Only whitespace may follow
          while (pos < len && isspace((unsigned char)str[pos])) ++pos;
          return pos == len && count == targetDimension;
        }

      if (pos >= len) break;

      // Parse one float: optional sign, digits, optional decimal, optional exponent
      size_t numStart = pos;
      if (pos < len && (str[pos] == '+' || str[pos] == '-')) ++pos;

      // Must have at least one digit
      if (pos >= len || !isdigit((unsigned char)str[pos])) return false;
      while (pos < len && isdigit((unsigned char)str[pos])) ++pos;

      // Optional decimal part
      if (pos < len && str[pos] == '.')
        {
          ++pos;
          while (pos < len && isdigit((unsigned char)str[pos])) ++pos;
        }

      // Optional exponent
      if (pos < len && (str[pos] == 'e' || str[pos] == 'E'))
        {
          ++pos;
          if (pos < len && (str[pos] == '+' || str[pos] == '-')) ++pos;
          if (pos >= len || !isdigit((unsigned char)str[pos])) return false;
          while (pos < len && isdigit((unsigned char)str[pos])) ++pos;
        }

      if (pos == numStart) return false;  // no number parsed
      ++count;

      // Early exit
      if (count > targetDimension) return false;
    }

  // No bracket case — must have consumed everything
  if (hasBracket) return false;  // missing closing ']'
  return count == targetDimension;
}

// Parse float array string into vector<float>
// Accepts bracketed or unbracketed, comma or whitespace separated
std::vector<float> floatArrayToFloat32(const std::string& str) ;


// Determine if string could be a float array (quick pre-check before
// the full isFloatArrayVector validation — avoids expensive scan)
inline bool couldBeFloatArray(const std::string_view str)
{
  if (str.empty()) return false;
  size_t pos = 0;
  while (pos < str.size() && isspace((unsigned char)str[pos])) ++pos;
  if (pos >= str.size()) return false;
  // Must start with '[', '-', '+', or a digit
  char c = str[pos];
  return c == '[' || c == '-' || c == '+' || isdigit((unsigned char)c);
}

} // namespace schmate_util
