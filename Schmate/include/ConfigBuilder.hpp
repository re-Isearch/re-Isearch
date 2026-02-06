#include "Logger.hpp"
#include "HnswConfig.hpp"

/*
Priority order (highest to lowest):

Project config - ./myproject/config.bin (project-specific)
Local config - ~/.schmate/config.bin (user preferences)
Global config - /etc/schmate/config.bin (system defaults)
Hardcoded defaults - In struct definition

Key features:

merge_overrides() - Only copies fields that differ from defaults
ConfigLoader - Handles the loading priority automatically
load_with_project() - Adds project-specific overrides

*/

// Builder pattern for easy construction
class HnswConfigBuilder {
private:
    hnswlib::HnswConfig cfg;

public:
    HnswConfigBuilder& with_max_elements(size_t n) { cfg.max_elements = n; return *this; }
    HnswConfigBuilder& with_M(size_t m) { cfg.M = m; return *this; }
    HnswConfigBuilder& with_ef_construction(size_t ef) { cfg.ef_construction = ef; return *this; }
    HnswConfigBuilder& with_ef_search(size_t ef) { cfg.ef_search = ef; return *this; }
    HnswConfigBuilder& with_metric(hnswlib::Metric m) { cfg.set_metric (m); return *this; }
    HnswConfigBuilder& with_search_mode(hnswlib::SearchModes m) { cfg.default_search_mode = m; return *this; }
    HnswConfigBuilder& with_debug(bool d) { cfg.debug = d; return *this; }
    HnswConfigBuilder& with_normalize_embeddings(bool n) { cfg.normalize_embeddings = n; return *this; }
    HnswConfigBuilder& with_default_k(size_t k) { cfg.default_k = k; return *this; }
    HnswConfigBuilder& with_epsilon(float e) { cfg.default_epsilon = e; return *this; }
    HnswConfigBuilder& with_epsilonL2(float e) { cfg.default_epsilonL2 = e; return *this; }
    HnswConfigBuilder& with_epsilonIP(float e) { cfg.default_epsilonIP = e; return *this; }

    HnswConfigBuilder& with_flush_threshold(int t) { cfg.flush_threshold = t; return *this; }
    
    hnswlib::HnswConfig build() {
        if (!cfg.validate()) {
            throw std::runtime_error("Invalid configuration");
        }
        return cfg;
    }
};

// Configuration loader with global defaults and local overrides
class ConfigLoader {
private:
    std::string global_config_path;
    std::string local_config_path;
    
public:
    ConfigLoader(const std::string& global_path = "/etc/ib/hnsw_config.bin",
                 const std::string& local_path = "~/.ib/hnsw_config.bin")
        : global_config_path(global_path)
        , local_config_path(expand_home(local_path))
    {}
    
    // Load configuration with priority: local > global > hardcoded defaults
    hnswlib::HnswConfig load(bool enable_debug = false) const {
        hnswlib::HnswConfig cfg;  // Start with hardcoded defaults
        if (enable_debug) cfg.debug = true;
        hnswlib::HnswConfig defaults = cfg;  // Save for comparison
        
        // Load global config (if exists)
        if (cfg.load_from_file(global_config_path)) {
            if (cfg.debug) LOG_DEBUG_S() << "Loaded global config from " << global_config_path;
            defaults = cfg;  // Update defaults to global config
        } else {
            if (cfg.debug) LOG_DEBUG_S() << "No global config found, using pre-set defaults";
        }
        
        // Load local config and override (if exists)
        hnswlib::HnswConfig local_cfg = cfg;  // Start with current config
        if (local_cfg.load_from_file(local_config_path)) {
            if (local_cfg.debug) LOG_DEBUG_S() << "Loaded local config from " << local_config_path;
            cfg.merge_overrides(local_cfg, defaults);
            if (cfg.debug) LOG_DEBUG_S()  << "Applied local overrides";
        } else {
            if (cfg.debug) LOG_DEBUG_S() << "No local config found";
        }
        
        return cfg;
    }
    
    // Load from specific project directory
    hnswlib::HnswConfig load_with_project(const std::string& project_dir) const {
        hnswlib::HnswConfig cfg = load();  // Start with global+local
        
        // Try to load project-specific config
        std::string project_config = project_dir + "/config.bin";
        hnswlib::HnswConfig project_cfg = cfg;
        if (project_cfg.load_from_file(project_config)) {
            if (cfg.debug) LOG_DEBUG_S() << "Loaded project config from " << project_config;
            hnswlib::HnswConfig base = cfg;
            cfg.merge_overrides(project_cfg, base);
            if (cfg.debug) LOG_DEBUG_S() << "Applied project overrides";
        }
        
        return cfg;
    }
    
    // Save as global default
    bool save_global(const hnswlib::HnswConfig& cfg) const {
        return cfg.save_to_file(global_config_path);
    }
    
    // Save as local override
    bool save_local(const hnswlib::HnswConfig& cfg) const {
        return cfg.save_to_file(local_config_path);
    }
    
private:
    static std::string expand_home(const std::string& path) {
        if (path.empty() || path[0] != '~') {
            return path;
        }
        const char* home = getenv("HOME");
        if (!home) home = getenv("USERPROFILE");  // Windows
        if (!home) return path;
        return std::string(home) + path.substr(1);
    }
};

/*
 * CONFIGURATION LOADING SYSTEM:
 * 
 * Priority order (highest to lowest):
 * 1. Project-specific config (./myproject/config.bin)
 * 2. Local user config (~/.hnsw/config.bin)
 * 3. Global system config (/etc/hnsw/config.bin)
 * 4. Hardcoded defaults (in struct definition)
 * 
 * USAGE:
 * 
 * // Simple loading (global + local)
 * ConfigLoader loader;
 * HnswConfig cfg = loader.load();
 * 
 * // With project-specific overrides
 * HnswConfig cfg = loader.load_with_project("./myproject");
 * 
 * // Save configurations
 * loader.save_global(cfg);   // Save as global defaults
 * loader.save_local(cfg);    // Save as local overrides
 * 
 * // Custom paths
 * ConfigLoader loader("/opt/app/config.bin", "~/.myapp/config.bin");
 * 
 * MERGE BEHAVIOR:
 * 
 * merge_overrides() only copies fields that differ from defaults
 * This allows:
 * - Global config sets: max_elements=10000, M=16
 * - Local config only sets: max_elements=50000
 * - Result: max_elements=50000, M=16
 * 
 * EXAMPLE WORKFLOW:
 * 
 * 1. System admin creates /etc/hnsw/config.bin with organization defaults
 * 2. User creates ~/.hnsw/config.bin with personal preferences
 * 3. Project creates ./config.bin with project-specific settings
 * 4. Application loads: project > user > system > hardcoded
 * 
 * FILE LOCATIONS:
 * 
 * Global:  /etc/hnsw/config.bin (system-wide)
 * Local:   ~/.hnsw/config.bin (user-specific)
 * Project: ./config.bin or ./myproject/config.bin (project-specific)
 * 
 * FEATURES:
 * 
 * 1. Validation:
 *    - validate() checks all parameters
 *    - Called after loading
 * 
 * 2. Helper methods:
 *    - get_epsilon(): Returns appropriate epsilon for current metric
 *    - get_max_candidates(): Applies cap if set
 *    - get_merge_threads(): Auto-detects if set to 0
 * 
 * 3. Serialization:
 *    - Binary save/load for persistence
 *    - Version marker for future compatibility
 * 
 * 4. Builder pattern:
 *    - Fluent interface for construction
 *    - Validates at build() time
 * 
 * 5. String conversions:
 *    - Parse from config files
 *    - Display in logs
 * 
 * 6. Three-level override system:
 *    - Global defaults
 *    - Local user overrides
 *    - Project-specific overrides
 */


/*


Example workflow:

Admin sets max_elements=10000 in global config
User sets max_elements=50000 in local config (overrides global)
Project sets metric=Cosine in project config
Final config: max_elements=50000, metric=Cosine, rest from global/defaults

The system intelligently merges only the changed values, so you don't need to duplicate all settings in each config file!



// Load global + local
ConfigLoader loader;
HnswConfig cfg = loader.load();

// With project-specific
HnswConfig cfg = loader.load_with_project("./myproject");

// Save configurations
loader.save_global(cfg);  // System-wide defaults
loader.save_local(cfg);   // User overrides

*/
