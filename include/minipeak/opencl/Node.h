#pragma once
#include "memory"
#include "Context.h"
#include "Kernel.h"
#include "minipeak/gpu/Node.h"



namespace OCL {
    cl::NDRange convert_range(GPU::DispatchRange range);

    class Node : virtual public GPU::Node {

    public:
        std::shared_ptr<Context> context;
        Kernel kernel;

        void operator()() override;
        Node() : context(Context::Inst()) {}

        int preferred_work_group_size_multiple() const override;
        int max_workgroup_size() const override;

        std::string platform_name() const override;

        std::string name() const override;

        void sync() override {
            context->queue.finish();
        }
        bool is_built() const override;

        void reset_build() override;
    };

class TileableNode : virtual public Node, virtual public GPU::TileableNode {
        public:

        TileableNode() : GPU::TileableNode() {}
        explicit TileableNode(bool is_tiled) : GPU::TileableNode(is_tiled) {}

    std::string name() const override;
};
}
