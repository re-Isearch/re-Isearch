#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>

struct EpsilonTuner {
    float epsilon_min = 0.1f;
    float epsilon_max = 1.0f;
    float epsilon = 0.3f;        // current epsilon radius
    size_t target_results = 5;   // desired number of matches per query
    double adjust_factor = 1.25; // growth/shrink factor
    double avg_result_count = 0; // smoothed moving average
    double smooth_alpha = 0.3;   // exponential smoothing factor

/*  
    | Situation                          | Behavior                             |
    | ---------------------------------- | ------------------------------------ |
    | Too few results (<80% of target)   | Increase ε slightly (expand radius). |
    | Too many results (>120% of target) | Shrink ε slightly (tighten radius).  |
    | Stable result density              | ε converges.                         |
*/  

    // === Adaptive adjustment ===
    void update_after_query(size_t result_count, bool debug = false) {

        // Exponential moving average for stability
        avg_result_count = (1.0 - smooth_alpha) * avg_result_count + smooth_alpha * result_count;

        if (avg_result_count < target_results * 0.8)
            epsilon = std::min(epsilon_max, float(epsilon * adjust_factor));
        else if (avg_result_count > target_results * 1.2)
            epsilon = std::max(epsilon_min, float(epsilon / adjust_factor));

        if (debug)
            std::cerr << "[EpsilonTuner] ε=" << epsilon
                      << " avg_results=" << avg_result_count
                      << " target=" << target_results << std::endl;
    }

    // === Persistence ===
    bool save(const std::string &path) const {
        std::ofstream ofs(path, std::ios::binary);
        return write(ofs);
    }
    bool write(std::ofstream &ofs) const {
        if (!ofs) return false;
        ofs.write(reinterpret_cast<const char*>(&epsilon), sizeof(epsilon));
        ofs.write(reinterpret_cast<const char*>(&epsilon_min), sizeof(epsilon_min));
        ofs.write(reinterpret_cast<const char*>(&epsilon_max), sizeof(epsilon_max));
        ofs.write(reinterpret_cast<const char*>(&target_results), sizeof(target_results));
        ofs.write(reinterpret_cast<const char*>(&adjust_factor), sizeof(adjust_factor));
        ofs.write(reinterpret_cast<const char*>(&avg_result_count), sizeof(avg_result_count));
        return true;
    }

    bool load(const std::string &path) {
        std::ifstream ifs(path, std::ios::binary);
        return read(ifs);
    }
    bool read(std::ifstream &ifs) {
        if (!ifs) return false;
        ifs.read(reinterpret_cast<char*>(&epsilon), sizeof(epsilon));
        ifs.read(reinterpret_cast<char*>(&epsilon_min), sizeof(epsilon_min));
        ifs.read(reinterpret_cast<char*>(&epsilon_max), sizeof(epsilon_max));
        ifs.read(reinterpret_cast<char*>(&target_results), sizeof(target_results));
        ifs.read(reinterpret_cast<char*>(&adjust_factor), sizeof(adjust_factor));
        ifs.read(reinterpret_cast<char*>(&avg_result_count), sizeof(avg_result_count));
        return true;
    }
};

