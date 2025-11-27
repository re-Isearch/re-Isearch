#pragma once

namespace hnswlib {

enum class QuantMode {
    NONE=0, BIN1=1, INT158=2, INT4=3, INT8=4, FP16=5, BF16 =7
};

// PASS means the Float32 vectors were already quantized!
enum class OptBinMode  { PASS=0, STANDARD, BETTER, CENTROID, ROTATIONAL, RABITQ, RABITQ_EXTENDED };

}

#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <thread>
#include <atomic>
#include <cassert>
#include <iostream>
#include <fstream>

#include "int_storage.h"
#include "hnswlib.h"

namespace hnswlib {


// Conversion functions
// ---------------------------------------------------------------------
// StorageType → actual bit-packing representation (BIN1, INT2, INT4, INT8, etc.)
// 
// QuantMode → higher-level quantization modes (binary, 1.58-bit, 4-bit, 8-bit).

inline std::optional<QuantMode> toQuantMode(StorageType st) noexcept { 
    switch (st) {        
        case StorageType::BIN1:
            return QuantMode::BIN1;    

        case StorageType::INT2:
        case StorageType::INT3:
            return QuantMode::INT158;  

        case StorageType::INT4:
        case StorageType::INT5:
            return QuantMode::INT4;

        case StorageType::INT6:
        case StorageType::INT8:
            return QuantMode::INT8;

        // There are technically not quantizations!
	case StorageType::FP16:
	    return QuantMode::FP16;
        case StorageType::BF16:
	    return QuantMode::BF16;

        case StorageType::FLOAT32:
            return QuantMode::NONE;
        default:
            return std::nullopt;   // INT16 → no quant mode
    }
}


inline std::optional<StorageType> toStorageType(QuantMode mode) noexcept {
    switch (mode) {
        case QuantMode::NONE:  return StorageType::FLOAT32;
        case QuantMode::BIN1:  return StorageType::BIN1;
        case QuantMode::INT158:return StorageType::INT2;
        case QuantMode::INT4:  return StorageType::INT4;
        case QuantMode::INT8:  return StorageType::INT8;
	case QuantMode::FP16:  return StorageType::FP16;
	case QuantMode::BF16:  return StorageType::BF16;
    }
}
// Convenience overload throwing on invalid type
inline QuantMode requireQuantMode(StorageType st) {
    auto q = toQuantMode(st);
    if (!q) throw std::invalid_argument("StorageType cannot be mapped to QuantMode");
    return *q;
}


} // namespace
