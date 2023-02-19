#pragma once

#include "minipeak/gpu/Node.h"
#include "program.h"

namespace OGL {
    struct Node : virtual public GPU::Node {
        GLSLProgram program;

        void sync() override;

        int max_workgroup_size() const override;

        int preferred_work_group_size_multiple() const override;

        std::string platform_name() const override;

        std::string name() const override;

        void operator()() override;

        bool is_built() const override { return (bool)program; }

        std::string preamble() const override;
    };

    class TileableNode : public Node, public GPU::TileableNode {
    public:
        std::string preamble() const override;

        TileableNode() : GPU::TileableNode() {}
        explicit TileableNode(std::optional<bool> is_tiled) : GPU::TileableNode(is_tiled) {}
    };
}