#include <iostream>
#include <cassert>
#include "minipeak/opencl/Node.h"
#include "numeric"
#include "CL/cl_ext.h"
#include "sstream"
#include "minipeak/gpu/Node.h"


void OCL::Node::operator()() {
    GPU::Node::operator()();
    try {
        context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, OCL::convert_range(global_size()), OCL::convert_range(work_size()));
    } catch (const cl::Error& error) {
        std::cerr << "Error in EnqueueNDRangeKernel " << error.what() << "(" << error.err() << " - " << OCL::errstr(error.err()) << ")" << std::endl;
        throw error;
    }
}

int OCL::Node::max_workgroup_size() const {
    return context->device[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
}

std::string OCL::Node::platform_name() const {
    return context->platform_name();
}

std::string OCL::Node::name() const {
    return kernel.name;
}

int OCL::Node::preferred_work_group_size_multiple() const {
    if(!is_built()) {
        const_cast<OCL::Node*>(this)->build();
    }
    return kernel.kernel.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(context->device[0]);
}

cl::NDRange OCL::convert_range(GPU::DispatchRange range) {
    return cl::NDRange(range[0], range[1], range[2]);
}
