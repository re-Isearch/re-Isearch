#include <sstream>
#include <iomanip>
#include "StringEmbedding.hpp"


// Encoding Vectors

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


// Convert now vectors to Hex encoded..

std::string binaryToHex(const std::vector<int>& values) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); i += 4) {
        uint8_t nibble = 0;
        for (size_t b = 0; b < 4 && (i + b) < values.size(); b++)
            nibble |= (values[i + b] & 1) << (3 - b);
        oss << std::hex << nibble;  // one hex char per nibble
    }
    return oss.str();
}

std::string int4ToHex(const std::vector<int>& values) {
    // Pack two int4 values per byte (high nibble, low nibble)
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); i += 2) {
        uint8_t hi = values[i]  & 0x0F;
        uint8_t lo = (i + 1 < values.size()) ? (values[i + 1] & 0x0F) : 0;
        uint8_t byte = (hi << 4) | lo;
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return oss.str();
}

std::string int8ToHex(const std::vector<int>& values) {
    std::ostringstream oss;
    for (auto v : values) {
        uint8_t byte = static_cast<uint8_t>(static_cast<int8_t>(v));
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return oss.str();
}

#if 0 /* Faster but les readable */

std::string float32ToHex(const std::vector<float>& v)
{
  static const char hex[] = "0123456789abcdef";
  std::string out;
  out.reserve(v.size() * 8);  // 8 hex chars per float32
  for (float f : v)
    {
      uint32_t bits;
      memcpy(&bits, &f, sizeof(bits));  // safe type-pun
      for (int i = 28; i >= 0; i -= 4)
        out.push_back(hex[(bits >> i) & 0xF]);
    }
  return out;
}

#else
std::string float32ToHex(const std::vector<float>& values) {
    std::ostringstream oss;
    for (auto v : values) {
        uint32_t bits;
        std::memcpy(&bits, &v, sizeof(bits));  // type-safe bit reinterpretation
        oss << std::hex << std::setw(8) << std::setfill('0') << bits;
    }
    return oss.str();
}
#endif


// Determine data type from base64 string length and target dimension
// Mirrors inferDataType for hex but accounts for base64 overhead.
// MongoDB BSON vector subtype 0x09 prepends 2 bytes (dtype + padding),
// so we check both with and without the 2-byte header.
DataType inferBase64DataType(const std::string& b64Str, size_t targetDimension)
{
  // Decode length: strip whitespace and padding to get raw b64 char count,
  // then compute byte count = b64chars * 3 / 4 (approximately)
  size_t b64Len = 0;
  size_t padLen = 0;
  for (char c : b64Str)
    {
      if (c == '=')           padLen++;
      else if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
                              b64Len++;
    }
  // Actual byte count encoded
  size_t byteCount = (b64Len * 3 / 4) - padLen;

  // MongoDB BSON vector subtype 0x09 has 2-byte header — strip it
  size_t effectiveBytes = (byteCount >= 2) ? byteCount - 2 : byteCount;

  // Check each quantization level (with and without BSON header)
  for (size_t bytes : { byteCount, effectiveBytes })
    {
      if (bytes * 8 == targetDimension)   return DataType::BINARY;
      if (bytes * 2 == targetDimension)   return DataType::INT4;
      if (bytes     == targetDimension)   return DataType::INT8;
      if (bytes     == targetDimension*4) return DataType::FLOAT32;
    }

  throw std::invalid_argument(
    "Cannot infer vector data type: base64 byte count " +
    std::to_string(byteCount) +
    " does not match dimension " +
    std::to_string(targetDimension) +
    " for any supported quantization");
}

// Base64 alphabet lookup table — faster than repeated comparisons
static const int8_t B64_TABLE[256] = {
  // -1 = invalid, -2 = padding, -3 = whitespace
  // Generated: A-Z=0-25, a-z=26-51, 0-9=52-61, +=62, /=63
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-3,-3,-1,-1,-3,-1,-1,  // 0x00-0x0F
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 0x10-0x1F
  -3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  // 0x20-0x2F (space,+,/)
  52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,  // 0x30-0x3F (0-9,=)
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, // 0x40-0x4F (A-O)
  15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  // 0x50-0x5F (P-Z)
  -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, // 0x60-0x6F (a-o)
  41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  // 0x70-0x7F (p-z)
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 0x80-0xFF (all invalid)
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};

// Core base64 decoder — returns raw bytes.
// Handles standard base64 (+/) and base64url (-_) via the table
// (caller swaps - and _ to + and / before passing, or we handle inline).
std::vector<uint8_t> base64Decode(const std::string& b64Str)
{
  std::vector<uint8_t> out;
  out.reserve(b64Str.size() * 3 / 4);

  uint32_t buf    = 0;
  int      bits   = 0;

  for (unsigned char c : b64Str)
    {
      // Handle base64url
      if (c == '-') c = '+';
      else if (c == '_') c = '/';

      int8_t v = B64_TABLE[c];
      if      (v == -3) continue;   // whitespace — skip
      else if (v == -2) break;      // padding — done
      else if (v == -1)
        throw std::invalid_argument(
          std::string("Invalid base64 character: ") + (char)c);

      buf  = (buf << 6) | (uint8_t)v;
      bits += 6;
      if (bits >= 8)
        {
          bits -= 8;
          out.push_back((buf >> bits) & 0xFF);
        }
    }
  return out;
}

// Implementations using the core decoder
std::vector<float> base64ToFloat32(const std::string& b64Str)
{
  auto bytes = base64Decode(b64Str);
  if (bytes.size() % 4 != 0)
    throw std::invalid_argument("base64ToFloat32: byte count not multiple of 4");
  std::vector<float> out(bytes.size() / 4);
  memcpy(out.data(), bytes.data(), bytes.size());
  return out;
}

std::vector<int> base64ToInt8(const std::string& b64Str)
{
  auto bytes = base64Decode(b64Str);
  std::vector<int> out;
  out.reserve(bytes.size());
  for (uint8_t b : bytes)
    out.push_back((int)(int8_t)b);  // reinterpret as signed
  return out;
}

std::vector<int> base64ToInt4(const std::string& b64Str)
{
  auto bytes = base64Decode(b64Str);
  std::vector<int> out;
  out.reserve(bytes.size() * 2);
  for (uint8_t b : bytes)
    {
      // High nibble first, sign-extend from 4 bits
      int hi = (b >> 4) & 0xF;  if (hi > 7) hi -= 16;
      int lo = (b)      & 0xF;  if (lo > 7) lo -= 16;
      out.push_back(hi);
      out.push_back(lo);
    }
  return out;
}

std::vector<int> base64ToBinary(const std::string& b64Str)
{
  auto bytes = base64Decode(b64Str);
  std::vector<int> out;
  out.reserve(bytes.size() * 8);
  for (uint8_t b : bytes)
    for (int bit = 7; bit >= 0; --bit)
      out.push_back((b >> bit) & 1);
  return out;
}


// Parse float array string into vector<float>
// Accepts bracketed or unbracketed, comma or whitespace separated
std::vector<float> floatArrayToFloat32(const std::string& str)
{
  std::vector<float> out;
  const char *s   = str.c_str();
  char       *end = nullptr;

  // Skip leading whitespace and optional '['
  while (*s && isspace((unsigned char)*s)) ++s;
  if (*s == '[') ++s;

  while (*s)
    {
      // Skip whitespace and commas
      while (*s && (isspace((unsigned char)*s) || *s == ',')) ++s;

      if (*s == ']' || *s == '\0') break;

      float v = strtof(s, &end);
      if (end == s) break;  // no progress — malformed
      out.push_back(v);
      s = end;
    }

  return out;
}



} // namespace schmate_util
