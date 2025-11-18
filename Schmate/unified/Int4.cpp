#include "int_storage.h"
#include <iostream>

/*
0: "F32",
1: "F16",
2: "Q4_0",
3: "Q4_1",
4: "Q4_1_SOME_F16"
7: "Q8_0",
8: "Q5_0",
9: "Q5_1",
10: "Q2_K",
11: "Q3_K_S",
12: "Q3_K_M",
13: "Q3_K_L",
14: "Q4_K_S",
15: "Q4_K_M",
16: "Q5_K_S",
17: "Q5_K_M",
18: "Q6_K",
19: "IQ2_XSS",
20: "IQ2_XS",
22: "IQ3_XS",
23: "IQ3_XXS",
24: "IQ1_S",
25: "IQ4_NL",
26: "IQ3_S",
27: "IQ3_M",
29: "IQ2_M",
30: "IQ4_XS",
31: "IQ1_M",
32: "BF16",
33: "Q4_0_4_4",
34: "Q4_0_4_8",
35: "Q4_0_8_8",
145: "IQ4_KS",
147: "IQ2_KS",
148: "IQ4_KSS",
150: "IQ5_KS",
154: "IQ3_KS",
155: "IQ2_KL",
156: "IQ1_KT"

*/

int main() {
    const size_t dim = 128;

    // Create both int8 and int4 storages
    hnswlib::IntStorage<float> s8(dim, hnswlib::QuantBits::INT8);
    hnswlib::IntStorage<float> s4(dim, hnswlib::QuantBits::INT4);

    // Generate random vector
    std::vector<float> v(dim);
    for (auto& x : v) x = float(rand()) / RAND_MAX;

    // Quantize to both formats
    auto e8 = s8.quantize(v);
    auto e4 = s4.quantize(v);

    // Save both to disk
    s8.save("embeddings.int8", {e8});
    s4.save("embeddings.int4", {e4});

    // Load them back
    auto l8 = s8.load("embeddings.int8");
    auto l4 = s4.load("embeddings.int4");

    // Dequantize first vector
    auto f8 = s8.dequantize(l8[0]);
    auto f4 = s4.dequantize(l4[0]);

    std::cout << "Original:   " << v[0]
              << "\nINT8-deq:  " << f8[0]
              << "\nINT4-deq:  " << f4[0] << std::endl;
}

