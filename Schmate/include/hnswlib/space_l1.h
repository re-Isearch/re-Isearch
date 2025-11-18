#pragma once
#include "hnswlib.h"

// ============================================
// L1 SPACE (Manhattan Distance)
// ============================================

namespace hnswlib {

class L1Space : public SpaceInterface<float> {
    DISTFUNC<float> fstdistfunc_;
    size_t data_size_;
    size_t dim_;

public:
    L1Space(size_t dim) {
        fstdistfunc_ = [](const void *pVect1v, const void *pVect2v, const void *qty_ptr) -> float {
            float *pVect1 = (float *) pVect1v;
            float *pVect2 = (float *) pVect2v;
            size_t qty = *((size_t *) qty_ptr);

            float res = 0;
            for (size_t i = 0; i < qty; i++) {
                float diff = pVect1[i] - pVect2[i];
                res += std::abs(diff);
            }
            return res;
        };
        
        dim_ = dim;
        data_size_ = dim * sizeof(float);
    }

    size_t get_bytes_per_vector() override {
       return  data_size_;
    }
    size_t get_data_size() override {
        return data_size_;
    }

    DISTFUNC<float> get_dist_func() override {
        return fstdistfunc_;
    }

    void *get_dist_func_param() override {
        return &dim_;
    }

    ~L1Space() {}
};


} // namespace hnswlib

