#pragma once
#include "memory"
#include "Context.h"
#include "Buffer.h"

namespace OCL {

    class Kernel {
        std::shared_ptr<Context> context;
    public:
        cl::Kernel kernel;
        cl::Program program;
        std::string name;

        Kernel() = default;
        Kernel(const std::string& name, const std::vector<std::string>& sources,
               const std::vector<Buffer>& input_buffers);
    };
}