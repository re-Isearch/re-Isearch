#include "test_data.hpp"

int main() {
    TestData data = generate_test_data(
        100,    // clusters
        1000,   // per cluster
        384,    // dimension
        50,     // queries
        0.05f,  // noise
        10      // ground truth k
    );

    std::cout << "Vectors: " << data.vectors.size() << "\n";
    std::cout << "Queries: " << data.queries.size() << "\n";
    std::cout << "GT rows: " << data.ground_truth.size() << "\n";
}

