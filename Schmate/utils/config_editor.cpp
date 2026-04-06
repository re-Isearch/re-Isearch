#include "../include/ConfigBuilder.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstring>

using namespace hnswlib;

class ConfigEditor {
private:
    HnswConfig cfg;
    std::string current_file;
    bool modified = false;

public:
    ConfigEditor() = default;

    void run() {
        std::cout << "=== HNSW Configuration Editor ===\n";
        std::cout << "Type 'help' for commands\n\n";
        
        std::string line;
        while (true) {
            std::cout << (modified ? "*" : "") << "> ";
            if (!std::getline(std::cin, line)) break;
            
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.empty()) continue;
            
            if (!process_command(line)) break;
        }
    }

private:
    bool process_command(const std::string& line) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "help" || cmd == "?") {
            print_help();
        }
        else if (cmd == "load") {
            std::string path;
            iss >> path;
            if (path.empty()) {
                std::cout << "Usage: load <file>\n";
            } else {
                load_file(path);
            }
        }
        else if (cmd == "save") {
            std::string path;
            iss >> path;
            save_file(path.empty() ? current_file : path);
        }
        else if (cmd == "new") {
            create_new();
        }
        else if (cmd == "list" || cmd == "ls") {
            list_all();
        }
        else if (cmd == "show" || cmd == "print") {
            std::string key;
            iss >> key;
            if (key.empty()) {
                cfg.print();
            } else {
                show_value(key);
            }
        }
        else if (cmd == "set") {
            std::string key, value;
            iss >> key;
            std::getline(iss, value);
            value.erase(0, value.find_first_not_of(" \t"));
            if (key.empty() || value.empty()) {
                std::cout << "Usage: set <key> <value>\n";
            } else {
                set_value(key, value);
            }
        }
        else if (cmd == "get") {
            std::string key;
            iss >> key;
            if (key.empty()) {
                std::cout << "Usage: get <key>\n";
            } else {
                show_value(key);
            }
        }
        else if (cmd == "reset") {
            reset_to_defaults();
        }
        else if (cmd == "validate") {
            validate_config();
        }
        else if (cmd == "export") {
            std::string path;
            iss >> path;
            if (path.empty()) {
                std::cout << "Usage: export <file.txt>\n";
            } else {
                export_text(path);
            }
        }
        else if (cmd == "import") {
            std::string path;
            iss >> path;
            if (path.empty()) {
                std::cout << "Usage: import <file.txt>\n";
            } else {
                import_text(path);
            }
        }
        else if (cmd == "search" || cmd == "find") {
            std::string pattern;
            iss >> pattern;
            if (pattern.empty()) {
                std::cout << "Usage: search <pattern>\n";
            } else {
                search_keys(pattern);
            }
        }
        else if (cmd == "diff") {
            std::string path;
            iss >> path;
            if (path.empty()) {
                std::cout << "Usage: diff <file>\n";
            } else {
                diff_with_file(path);
            }
        }
        else if (cmd == "quit" || cmd == "exit" || cmd == "q") {
            if (modified) {
                std::cout << "Unsaved changes! Save first or use 'quit!' to discard.\n";
                return true;
            }
            return false;
        }
        else if (cmd == "quit!" || cmd == "exit!" || cmd == "q!") {
            return false;
        }
        else {
            std::cout << "Unknown command: " << cmd << "\n";
            std::cout << "Type 'help' for available commands\n";
        }
        
        return true;
    }

    void print_help() {
        std::cout << "\nAvailable commands:\n";
        std::cout << "  help                  - Show this help\n";
        std::cout << "  load <file>          - Load configuration from binary file\n";
        std::cout << "  save [file]          - Save configuration (to current or new file)\n";
        std::cout << "  new                  - Create new config with defaults\n";
        std::cout << "  list                 - List all configuration keys\n";
        std::cout << "  show [key]           - Show all values or specific key\n";
        std::cout << "  get <key>            - Get value of specific key\n";
        std::cout << "  set <key> <value>    - Set value for key\n";
        std::cout << "  reset                - Reset to default values\n";
        std::cout << "  validate             - Validate current configuration\n";
        std::cout << "  export <file.txt>    - Export as text file\n";
        std::cout << "  import <file.txt>    - Import from text file\n";
        std::cout << "  search <pattern>     - Search for keys matching pattern\n";
        std::cout << "  diff <file>          - Compare with another config file\n";
        std::cout << "  quit / q             - Exit (prompts if unsaved)\n";
        std::cout << "  quit! / q!           - Exit without saving\n";
        std::cout << "\nExamples:\n";
        std::cout << "  set max_elements 100000\n";
        std::cout << "  set metric Cosine\n";
        std::cout << "  set debug true\n";
        std::cout << "  get max_elements\n";
        std::cout << "\n";
    }

    void load_file(const std::string& path) {
        if (modified) {
            std::cout << "Unsaved changes! Save first? (y/n): ";
            std::string answer;
            std::getline(std::cin, answer);
            if (answer == "y" || answer == "yes") {
                save_file(current_file);
            }
        }
        
        if (cfg.load_from_file(path)) {
            current_file = path;
            modified = false;
            std::cout << "Loaded config from: " << path << "\n";
        } else {
            std::cout << "Failed to load: " << path << "\n";
        }
    }

    void save_file(const std::string& path) {
        if (path.empty()) {
            std::cout << "No file specified. Use: save <file>\n";
            return;
        }
        
        if (cfg.save_to_file(path)) {
            current_file = path;
            modified = false;
            std::cout << "Saved config to: " << path << "\n";
        } else {
            std::cout << "Failed to save: " << path << "\n";
        }
    }

    void create_new() {
        if (modified) {
            std::cout << "Discard current changes? (y/n): ";
            std::string answer;
            std::getline(std::cin, answer);
            if (answer != "y" && answer != "yes") return;
        }
        
        cfg = HnswConfig();
        current_file.clear();
        modified = false;
        std::cout << "Created new configuration with defaults\n";
    }

    void list_all() {
        std::cout << "\nConfiguration keys:\n";
        std::cout << std::string(60, '-') << "\n";
        
        auto keys = HnswConfig::get_all_keys();
        std::sort(keys.begin(), keys.end());
        
        for (const auto& key : keys) {
            try {
                std::string value = cfg.get(key);
                std::cout << "  " << std::left << std::setw(25) << key 
                          << " = " << value << "\n";
            } catch (...) {
                std::cout << "  " << key << " = <error>\n";
            }
        }
        std::cout << std::string(60, '-') << "\n";
        std::cout << "Total: " << keys.size() << " keys\n\n";
    }

    void show_value(const std::string& key) {
        try {
            std::string value = cfg.get(key);
            std::cout << key << " = " << value << "\n";
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }

    void set_value(const std::string& key, const std::string& value) {
        if (cfg.set(key, value)) {
            modified = true;
            std::cout << "Set " << key << " = " << value << "\n";
        } else {
            std::cout << "Failed to set " << key << "\n";
        }
    }

    void reset_to_defaults() {
        std::cout << "Reset to default values? (y/n): ";
        std::string answer;
        std::getline(std::cin, answer);
        if (answer == "y" || answer == "yes") {
            cfg = HnswConfig();
            modified = true;
            std::cout << "Reset to defaults\n";
        }
    }

    void validate_config() {
        std::cout << "Validating configuration...\n";
        if (cfg.validate()) {
            std::cout << "✓ Configuration is valid\n";
        } else {
            std::cout << "✗ Configuration has errors (see above)\n";
        }
    }

    void export_text(const std::string& path) {
        std::ofstream ofs(path);
        if (!ofs) {
            std::cout << "Failed to open: " << path << "\n";
            return;
        }
        
        ofs << "# HNSW Configuration\n";
        ofs << "# Generated by config editor\n\n";
        
        auto keys = HnswConfig::get_all_keys();
        std::sort(keys.begin(), keys.end());
        
        for (const auto& key : keys) {
            try {
                ofs << key << " = " << cfg.get(key) << "\n";
            } catch (...) {}
        }
        
        std::cout << "Exported to: " << path << "\n";
    }

    void import_text(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs) {
            std::cout << "Failed to open: " << path << "\n";
            return;
        }
        
        std::string line;
        int count = 0;
        int errors = 0;
        
        while (std::getline(ifs, line)) {
            // Skip comments and empty lines
            line.erase(0, line.find_first_not_of(" \t"));
            if (line.empty() || line[0] == '#') continue;
            
            // Parse key = value
            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;
            
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (cfg.set(key, value)) {
                count++;
            } else {
                errors++;
            }
        }
        
        modified = (count > 0);
        std::cout << "Imported " << count << " settings";
        if (errors > 0) {
            std::cout << " (" << errors << " errors)";
        }
        std::cout << "\n";
    }

    void search_keys(const std::string& pattern) {
        auto keys = HnswConfig::get_all_keys();
        std::vector<std::string> matches;
        
        for (const auto& key : keys) {
            if (key.find(pattern) != std::string::npos) {
                matches.push_back(key);
            }
        }
        
        if (matches.empty()) {
            std::cout << "No keys matching: " << pattern << "\n";
        } else {
            std::cout << "Found " << matches.size() << " matching keys:\n";
            for (const auto& key : matches) {
                std::cout << "  " << std::left << std::setw(25) << key 
                          << " = " << cfg.get(key) << "\n";
            }
        }
    }

    void diff_with_file(const std::string& path) {
        HnswConfig other;
        if (!other.load_from_file(path)) {
            std::cout << "Failed to load: " << path << "\n";
            return;
        }
        
        std::cout << "Differences with " << path << ":\n";
        std::cout << std::string(70, '-') << "\n";
        
        auto keys = HnswConfig::get_all_keys();
        int diff_count = 0;
        
        for (const auto& key : keys) {
            try {
                std::string val1 = cfg.get(key);
                std::string val2 = other.get(key);
                
                if (val1 != val2) {
                    std::cout << "  " << std::left << std::setw(25) << key 
                              << " : " << std::setw(15) << val1 
                              << " -> " << val2 << "\n";
                    diff_count++;
                }
            } catch (...) {}
        }
        
        if (diff_count == 0) {
            std::cout << "  (no differences)\n";
        } else {
            std::cout << std::string(70, '-') << "\n";
            std::cout << "Total differences: " << diff_count << "\n";
        }
    }
};

// Command-line interface
int main(int argc, char* argv[]) {
#ifdef __APPLE__
    extern void relax_macos_malloc_zones();
    relax_macos_malloc_zones();
#endif
    ConfigEditor editor;

    // Verify struct size is reasonable
    // std::cout << "HnswConfig size: " << sizeof(HnswConfig) << " bytes\n";
    
    // Handle command-line arguments
    if (argc > 1) {
        std::string cmd = argv[1];
        
        if (cmd == "--help" || cmd == "-h") {
            std::cout << "HNSW Configuration Editor\n";
            std::cout << "Usage:\n";
            std::cout << "  " << argv[0] << "              - Interactive mode\n";
            std::cout << "  " << argv[0] << " <file>       - Load and edit file\n";
            std::cout << "  " << argv[0] << " new <file>   - Create new config\n";
            std::cout << "  " << argv[0] << " show <file>  - Display config\n";
            std::cout << "  " << argv[0] << " export <in> <out> - Export to text\n";
            std::cout << "\nInteractive commands available once started.\n";
            return 0;
        }
        else if (cmd == "new" && argc > 2) {
            HnswConfig cfg;
            if (cfg.save_to_file(argv[2])) {
                std::cout << "Created new config: " << argv[2] << "\n";
            }
            return 0;
        }
        else if (cmd == "show" && argc > 2) {
            HnswConfig cfg;
            if (cfg.load_from_file(argv[2])) {
                cfg.print();
            }
            return 0;
        }
        else if (cmd == "export" && argc > 3) {
            HnswConfig cfg;
            if (cfg.load_from_file(argv[2])) {
                std::ofstream ofs(argv[3]);
                auto keys = HnswConfig::get_all_keys();
                for (const auto& key : keys) {
                    ofs << key << " = " << cfg.get(key) << "\n";
                }
                std::cout << "Exported to: " << argv[3] << "\n";
            }
            return 0;
        }
        else {
            // Assume it's a file to load
            // Will be handled in interactive mode
        }
    }
    
    editor.run();
    return 0;
}
