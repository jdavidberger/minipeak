#pragma once

#include "minipeak/gpu/Buffer.h"
#include "minipeak/opencl/Context.h"

namespace OCL {
    class Buffer : public GPUBuffer {
        std::shared_ptr<Context> context;
    public:
        bool is_host_accessible() const;

        std::shared_ptr<cl::Buffer> b;
        Buffer(const BufferInfo_t& info);
        template <typename... Args>
        Buffer(Args... args) : Buffer(BufferInfo_t(args...)) {}

        std::shared_ptr<GPUBuffer> clone() const override;

        void read(void *dst) const override;

        void write(const void *src) override;

        std::string cl_constants(const std::string& prefix = "", int vec_size = 1) const;

        void stage_read(void *dst) const override;

        void stage_write(const void *src) override;
    };
}