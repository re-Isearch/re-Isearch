#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include "Logger.hpp"

struct EfSearchTuner {
    size_t ef_min = 64;
    size_t ef_max = 800;
    double target_latency_ms = 20.0;
    double adjust_factor = 1.25;
    double last_query_time_ms = 0.0;
    size_t current_ef = 200;

    EfSearchTuner(size_t initial = 200) : current_ef(initial) {}

/*
    | Use Case                           | ef_search | Description                                               |
    | ---------------------------------- | --------- | --------------------------------------------------------- |
    | 🔹 **Speed-critical (real-time)**  | `16–64`   | Fast, but may miss some near neighbors.                   |
    | 🔹 **Balanced (default)**          | `100–200` | Common sweet spot (used by FAISS, sentence-transformers). |
    | 🔹 **High recall / offline batch** | `300–800` | High accuracy, slower query.                              |
    | 🔹 **Max recall / evaluation**     | `1000+`   | Nearly exact results, but 10× slower.                     |
*/

    // === Adaptive update ===
    void update_after_query(double elapsed_ms, size_t num_elements, bool debug = false) {
        last_query_time_ms = elapsed_ms;

        if (elapsed_ms > target_latency_ms * 1.2) {
            current_ef = std::max(ef_min, size_t(current_ef / adjust_factor));
        } else if (elapsed_ms < target_latency_ms * 0.8) {
            current_ef = std::min(ef_max, size_t(current_ef * adjust_factor));
        } else if (num_elements && last_query_time_ms == 0.0) {
            current_ef = std::clamp(size_t(std::sqrt(num_elements) * 4), ef_min, ef_max);
        }

        if (debug) {
            LOG_INFO_S() << "[EfTuner] ef=" << current_ef
                      << " elapsed=" << elapsed_ms
                      << " target=" << target_latency_ms;
        }
    }

    // === Persistence ===
    bool save(const std::string &path) const {
        std::ofstream ofs(path, std::ios::binary);
	return write(ofs);
    }
    bool write(std::ofstream &ofs) const {
        if (!ofs) return false;
        ofs.write(reinterpret_cast<const char*>(&ef_min), sizeof(ef_min));
        ofs.write(reinterpret_cast<const char*>(&ef_max), sizeof(ef_max));
        ofs.write(reinterpret_cast<const char*>(&target_latency_ms), sizeof(target_latency_ms));
        ofs.write(reinterpret_cast<const char*>(&adjust_factor), sizeof(adjust_factor));
        ofs.write(reinterpret_cast<const char*>(&last_query_time_ms), sizeof(last_query_time_ms));
        ofs.write(reinterpret_cast<const char*>(&current_ef), sizeof(current_ef));
        return true;
    }

    bool load(const std::string &path) {
        std::ifstream ifs(path, std::ios::binary);
        return read(ifs);
    }
    bool read(std::ifstream &ifs) {
        if (!ifs) return false;

        size_t ef = current_ef;
        ifs.read(reinterpret_cast<char*>(&ef_min), sizeof(ef_min));
        ifs.read(reinterpret_cast<char*>(&ef_max), sizeof(ef_max));
        ifs.read(reinterpret_cast<char*>(&target_latency_ms), sizeof(target_latency_ms));
        ifs.read(reinterpret_cast<char*>(&adjust_factor), sizeof(adjust_factor));
        ifs.read(reinterpret_cast<char*>(&last_query_time_ms), sizeof(last_query_time_ms));
        ifs.read(reinterpret_cast<char*>(&current_ef), sizeof(current_ef));
        if (current_ef == 0) current_ef = ef; // Use pre-set
        return true;
    }
};
