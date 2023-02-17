#pragma once
#include "memory"
#include "Context.h"
#include "Kernel.h"

namespace OCL {
    class Node {
    protected:
        mutable cl::NDRange set_ws = {0};
    public:
        std::shared_ptr<Context> context;
        Kernel kernel;
        virtual cl::NDRange global_size() const = 0;
        virtual cl::NDRange work_size() const;
        virtual void operator()();
        Node() : context(Context::Inst()) {}

        virtual std::string preamble() const { return ""; }

        void set_work_size(const cl::NDRange& ws);
        bool is_ws_valid(const cl::NDRange& ws) const;

        virtual void sync() {
            context->queue.finish();
        }
    };

    class TileableNode : public Node {
    public:
        bool is_tiled = true;

        TileableNode();
        TileableNode(bool is_tiled);
        std::string preamble() const override;

        cl::NDRange global_size() const override;
        virtual cl::NDRange global_tile_size() const = 0;
    };
}
