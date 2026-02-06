#pragma once

#include <cstddef>

#include "HnswConfig.hpp"
#include "EfSearchTuner.hpp"
#include "epsilonTuner.hpp"
#include "Logger.hpp"

/// Unified controller that manages adaptive tuning for both ef_search (kNN)
/// and epsilon (radius / range) search strategies.
class AdaptiveSearchController {
public:
    EfSearchTuner ef_tuner;
    EpsilonTuner eps_tuner;

    bool adaptive_ef = true;
    bool adaptive_epsilon = true;

    bool combined = true;

    AdaptiveSearchController() = default;

    // === Persistence ===
    void save(const std::string &base_path) const {
        if (adaptive_ef)
            ef_tuner.save(base_path + hnswlib::IndexFileExtensions::tuner);
        if (adaptive_epsilon)
            eps_tuner.save(base_path + hnswlib::IndexFileExtensions::eps);
    }

    bool load(const std::string &base_path) {
        bool ok = false;
        if (adaptive_ef)
            ok |= ef_tuner.load(base_path + hnswlib::IndexFileExtensions::tuner);
        if (adaptive_epsilon)
            ok |= eps_tuner.load(base_path + hnswlib::IndexFileExtensions::eps);
        return ok;
    }

    // === Adaptive control interface ===

    /// Called after a kNN query completes.
    /// @param latency_ms measured time per query
    /// @param debug enable logs
    void update_after_knn(float latency_ms, bool debug = false) {
        if (adaptive_ef)
            ef_tuner.update_after_query(latency_ms, debug);
    }

    /// Called after a radius (epsilon) query completes.
    /// @param result_count number of results returned
    /// @param debug enable logs
    void update_after_epsilon(size_t result_count, bool debug = false) {
        if (adaptive_epsilon)
            eps_tuner.update_after_query(result_count, debug);
    }

    /// Get the current efSearch value to use for next query
    size_t get_ef() const {
        return ef_tuner.current_ef;
    }
    void set_ef(size_t val) {
        ef_tuner.current_ef = val;
    }
    void set_latency(double val) {
        ef_tuner.last_query_time_ms = val;
    }
    double get_latency() const {
        return ef_tuner.last_query_time_ms;
    }

    /// Get the current epsilon radius to use for next query
    float get_epsilon() const {
        if (!adaptive_epsilon) return 0.0f;
        return eps_tuner.epsilon;
    }
    void set_epsilon(float val) {
        if (val > 0.0f) eps_tuner.epsilon = val;
    }

    /// Print current tuning status (for debugging / UI)
    void print_status(std::ostream& os = std::cout) const {
        os  << "[AdaptiveSearchController]\n"
                  << "  ef_search: " << get_ef()
                  << " (auto=" << adaptive_ef << ")\n"
                  << "  epsilon:   " << get_epsilon() 
                  << " (auto=" << adaptive_epsilon << ")\n";
    }
};

