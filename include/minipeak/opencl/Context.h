#pragma once
#include "memory"
#define CL_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_ENABLE_EXCEPTIONS 1
#include <CL/cl2.hpp>
#include "minipeak/gpu/BufferInfo.h"

namespace OCL {
  const char *errstr(cl_int err);
  class Context {
        static std::weak_ptr<Context> g;

        Context();

    public:
        cl::CommandQueue queue;
        cl::Context context;
        std::vector<cl::Device> device;

        std::shared_ptr<cl::Buffer> AllocBuffer(const BufferInfo_t& info);

        ~Context();
        static std::shared_ptr<Context> Inst();
    };
}
