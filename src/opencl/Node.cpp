//
// Created by justin on 1/19/23.
//

#include <iostream>
#include "minipeak/opencl/Node.h"
#include "numeric"
#include "CL/cl_ext.h"

void OCL::Node::operator()() {
    try {
        context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, global_size(), work_size());
    } catch (const cl::Error& error) {
        std::cerr << "Error in EnqueueNDRangeKernel " << error.what() << "(" << error.err() << " - " << OCL::errstr(error.err()) << ")" << std::endl;
        throw error;
    }
}

cl::NDRange OCL::Node::work_size() const {
    if(cache_ws[0] != 0) return cache_ws;

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
    return cache_ws = cl::NDRange(range[0], range[1], range[2]);
}
