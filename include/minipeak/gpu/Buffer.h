#pragma once

#include "memory"
#include "BufferInfo.h"
#include <vector>
#include <cassert>
#include <opencv2/core/mat.hpp>

class GPUBuffer {
protected:

    void *mapped_ptr = 0;
public:
    BufferInfo_t info;

    virtual std::shared_ptr<GPUBuffer> clone() const = 0;

    virtual ~GPUBuffer() = default;
    GPUBuffer() = default;
    GPUBuffer(const BufferInfo_t& info) : info(info) {}

    size_t type_size() const { return info.type_size(); }
    size_t buffer_size() const { return info.buffer_size(); }

    virtual void read(void* dst) const = 0;
    virtual void write(const void* src) = 0;

    void write_mat(const cv::Mat& src, float int_scale = 1) {
        if(src.elemSize() != sizeof(float)) {
            cv::Mat out;
            src.convertTo(out, CV_32F, 1./int_scale);
            write_mat(out);
            return;
        }
        write(src.data);
    }

    void write_vector(const std::vector<float>& src) {
        assert(info.type == 'f');
        assert(src.size() * sizeof(float) == buffer_size());
        write(src.data());
    }

    virtual void stage_read(void* dst) const { read(dst); }
    virtual void stage_write(const void* src) { write(src); }

    virtual bool is_host_visible() const {
        return mapped_ptr != 0;
    }

    bool is_uniform() const { return info.uniform(); }
};
