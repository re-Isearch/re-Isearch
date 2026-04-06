#include "GGUFParser.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " model.gguf|model.ggml\n";
        return 1;
    }

    try {
        GGUFParser parser(argv[1]);
        if (!parser.is_valid()) {
            std::cerr << "Invalid or unknown file format.\n";
            return 1;
        }
        parser.print_info();
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

