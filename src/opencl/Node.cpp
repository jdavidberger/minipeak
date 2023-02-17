#include <iostream>
#include <cassert>
#include "minipeak/opencl/Node.h"
#include "numeric"
#include "CL/cl_ext.h"
#include "sstream"

void OCL::Node::operator()() {
    try {
        context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, global_size(), work_size());
    } catch (const cl::Error& error) {
        std::cerr << "Error in EnqueueNDRangeKernel " << error.what() << "(" << error.err() << " - " << OCL::errstr(error.err()) << ")" << std::endl;
        throw error;
    }
}

cl::NDRange OCL::Node::work_size() const {
    if(set_ws[0] != 0) return set_ws;

#ifdef cl_khr_suggested_local_work_size
aef
#endif

    auto run_size = global_size();
    auto max_local_size = std::min(512ul, context->device[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>());
    auto next_local_size = max_local_size;
    int range[3] = { 0 };
    for(int i = 0;i < 3;i++) {
        range[i] = std::gcd(run_size[i], next_local_size);
        next_local_size = next_local_size / range[i];
    }

    printf("Node %s has global size of %zu, %zu, %zu and local size of %d, %d, %d (max of %lu)\n", kernel.name.c_str(),
           run_size[0], run_size[1], run_size[2], range[0], range[1], range[2], max_local_size);
    return set_ws = cl::NDRange(range[0], range[1], range[2]);
}

bool OCL::Node::is_ws_valid(const cl::NDRange &ws) const {
    auto gs = global_size();
    for(int i = 0;i < 3;i++) {
        if((gs[i] % ws[i]) != 0)
            return false;
    }
    return true;
}

void OCL::Node::set_work_size(const cl::NDRange &ws) {
    if(ws[0] != 0) {
        assert(is_ws_valid(ws));
        set_ws = ws;
    }
}

std::string OCL::TileableNode::preamble() const {
    std::stringstream ss;
    ss << "#define IS_TILED " << is_tiled << std::endl;
    auto gts = this->global_tile_size();
    if(is_tiled) {
        ss << "#define get_tile_x() get_global_id(0) " << std::endl;
        ss << "#define get_tile_y() get_global_id(1) " << std::endl;
        ss << "#define get_tile_z() get_global_id(2) " << std::endl;
        ss << "#define get_tile_idx() (get_global_id(0) + get_global_id(1) * " << (gts[0]) << "+ get_global_id(2) * " << (gts[0] * gts[1]) << ") " << std::endl;
    } else {
        ss << "#define get_tile_x() (get_global_id(0) % " << gts[0] << ") " << std::endl;
        ss << "#define get_tile_y() ((get_global_id(0) / " << gts[0] << ") % " << gts[1] << ")" << std::endl;
        ss << "#define get_tile_z() (get_global_id(0) / (" << gts[0] * gts[1] << "))" << std::endl;
        ss << "#define get_tile_idx() get_global_id(0)" << std::endl;
    }
    return ss.str();
}

OCL::TileableNode::TileableNode() {}

OCL::TileableNode::TileableNode(bool is_tiled) : is_tiled(is_tiled){}

cl::NDRange OCL::TileableNode::global_size() const {
    if(is_tiled) {
        return global_tile_size();
    } else {
        auto gts = global_tile_size();
        return {gts[0] * gts[1] * gts[2], 1 ,1};
    }
}
