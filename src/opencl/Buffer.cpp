#include <cassert>
#include "minipeak/opencl/Buffer.h"
#include "sstream"

OCL::Buffer::Buffer(const BufferInfo_t &info) : GPUBuffer(info), context(Context::Inst()) {
    b = context->AllocBuffer(info);
}

std::shared_ptr<GPUBuffer> OCL::Buffer::clone() const {
    return std::shared_ptr<GPUBuffer>();
}

void OCL::Buffer::read(void *dst) const {
    if(dst == 0) return;
    if(!is_host_accessible()) return;
    //cl::copy((uint8_t*)dst, (uint8_t*)dst + info.buffer_size(), *b);
    context->queue.enqueueReadBuffer(*b, true, 0, info.buffer_size(), dst);
}

void OCL::Buffer::write(const void *src) {
    if(src == 0) return;
    if(!is_host_accessible()) return;
    //cl::copy(*b, (uint8_t*)src, (uint8_t*)src + info.buffer_size());
    context->queue.enqueueWriteBuffer(*b, true, 0, info.buffer_size(), src);
}

void OCL::Buffer::stage_read(void *dst) const {
    if(dst == 0) return;
    if(!is_host_accessible()) return;
    //cl::copy((uint8_t*)dst, (uint8_t*)dst + info.buffer_size(), *b);
    context->queue.enqueueReadBuffer(*b, false, 0, info.buffer_size(), dst);
}

void OCL::Buffer::stage_write(const void *src) {
    if(src == 0) return;
    if(!is_host_accessible()) return;
    //cl::copy(*b, (uint8_t*)src, (uint8_t*)src + info.buffer_size());
    context->queue.enqueueWriteBuffer(*b, false, 0, info.buffer_size(), src);
}

std::string OCL::Buffer::cl_constants(const std::string &prefix, int vec_size) const {
    auto order = info.order;
    auto type = info.type;
    if ((info.w % vec_size) != 0) {
        throw std::runtime_error("Invalid vectorization size");
    }

    std::stringstream ss;
    ss << "#define " << prefix << "dtype " << info.cl_type(vec_size) << std::endl;
    ss << "#define " << prefix << "order " << (int)order << std::endl;
    if(order == BufferInfo_t::order_t::CHW) {
        ss << "#define " << prefix << "ACCESS(d,c,x,y) d[c][x][y]" << std::endl;
        ss << "#define " << prefix << "FLAT_ACCESS(d,c,x,y) d[((c) * (" << prefix << "width * " << prefix
           << "height) + (x) * (" << prefix << "width) + (y))]" << std::endl;
    } else {
        ss << "#define " << prefix << "ACCESS(d,c,x,y) d[x][y][c]" << std::endl;
        ss << "#define " << prefix << "FLAT_ACCESS_IDX(c,x,y) (((x) * ("<< prefix <<"width * "<< prefix <<"channels) + (y) * ("<< prefix <<"channels) + (c)))" << std::endl;
        ss << "#define " << prefix << "FLAT_ACCESS(d,c,x,y) d[((x) * ("<< prefix <<"width * "<< prefix <<"channels) + (y) * ("<< prefix <<"channels) + (c))]" << std::endl;
    }
    auto c = info.c;
    auto w = info.w;
    auto h = info.h;

    ss << "#define convert_" << prefix << " convert_" << info.cl_type(vec_size) << std::endl;
    ss << "#define " << prefix << "channels ((ushort)" << c << "u)" << std::endl;
    ss << "#define " << prefix << "width ((ushort)" << (w/vec_size) << "u)" << std::endl;
    ss << "#define " << prefix << "height ((ushort)" << h << "u)" << std::endl;
    return ss.str();
}

bool OCL::Buffer::is_host_accessible() const {
    auto flags = b->getInfo<CL_MEM_FLAGS>();
    return (flags & CL_MEM_HOST_NO_ACCESS) != CL_MEM_HOST_NO_ACCESS;
}

