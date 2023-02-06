#pragma once
#include "memory"
#include "Context.h"
#include "Kernel.h"

namespace OCL {
    class Node {
    protected:
        std::shared_ptr<Context> context;
        mutable cl::NDRange cache_ws = {0};
    public:
        Kernel kernel;
        virtual cl::NDRange global_size() const = 0;
        virtual cl::NDRange work_size() const;
        virtual void operator()();
        Node() : context(Context::Inst()) {}

        virtual void sync() {
            context->queue.finish();
        }
    };
}