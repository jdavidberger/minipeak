#include <iostream>
#include <chrono>
#include "timeit.h"
#include "zlib.h"
#include <sstream>

#include <minipeak/gpu/BufferInfo.h>
#include <minipeak/opencl/Buffer.h>
#include <minipeak/opencl/Node.h>

#include <vector>

static std::string source = R"18e3c792b59(

__kernel void memread(MODIFIER const float *inptr, __global float *ptr, uint access_stride)
{
    uint id = get_global_id(0);
    float y = (float)((float)(id) * 1e-10);

    for(uint i = 0u;i < width*height;i++) {
       y += inptr[(i * access_stride + id) % (width*height)];
    }

    ptr[get_global_id(0)] = y;
}

)18e3c792b59";

int main(int argc, char **argv) {
    auto context = OCL::Context::Inst();

    uint64_t localSize = 256;
    if(argc > 1) {
        localSize = atoll(argv[1]);
    }
    uint64_t globalWIs = 32 * localSize;
    auto out_buffer = OCL::Buffer('f', globalWIs, 1, 1, BufferInfo_t::usage_t::intermediate);

    auto memaccess = std::vector<std::pair<std::string, int>> {
            {"linear", 1},
            {"random", 1223},
    };

#define CL_USAGE_FLAG(x) { #x, OCL::Context::GetFlagFromUsage(BufferInfo_t::usage_t::x) }
#define CL_MEM_FLAG(x) { #x, x }
    auto usages = std::vector<std::tuple<std::string, cl_mem_flags>> {
            CL_USAGE_FLAG(intermediate),
            CL_USAGE_FLAG(input),
            CL_USAGE_FLAG(output),
            CL_USAGE_FLAG(uniform),
            CL_USAGE_FLAG(host_available),	    
            CL_MEM_FLAG(CL_MEM_READ_ONLY),
    };

    int width = 128, height = 128;

    for(auto& modifier : {"__global", "__constant"}) {
        for (auto &k: usages) {
            auto name = std::get<0>(k);
            auto buffer = OCL::Buffer(BufferInfo_t('f', 1, width, height), std::get<1>(k));
            std::vector<uint8_t> d;
            d.resize(buffer.buffer_size());
            for (auto &v: d) {
                v = 0xce;
            }
            if (buffer.host_can_write())
                buffer.write(d.data());

            auto flags = buffer.b->getInfo<CL_MEM_FLAGS>();
            printf("%s: (flags 0x%x) %s \n", name.c_str(), flags, modifier);

            if (buffer.host_can_read()) {
                Timeit t("\tread", width * height * 4 / 1024. / 1024., "MBs");
                do {
                    buffer.read(d.data());
                } while (t.under(1));
            }

            if (buffer.host_can_write()) {
                Timeit t("\twrite", width * height * 4 / 1024. / 1024., "MBs");
                do {
                    buffer.write(d.data());
                } while (t.under(1));
            }

            for (auto &m: memaccess) {

                auto kernel = OCL::Kernel("memread", {std::string("#define MODIFIER ") + modifier + "\n", buffer.cl_constants(), source}, {buffer, out_buffer});
                kernel.kernel.setArg(2, m.second);

                context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, globalWIs, localSize);
                context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, globalWIs, localSize);
                context->queue.finish();

                {
                    Timeit t("\t" + m.first, width * height * 4 * (double) (globalWIs) / 1024. / 1024., "MBs");
                    do {
                        context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, globalWIs, localSize);
                        context->queue.finish();
                    } while (t.under(1));
                }
            }
        }
    }
    return 0;
}
