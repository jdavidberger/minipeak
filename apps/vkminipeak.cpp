#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <zlib.h>

#include "minipeak/vulkan/VBuffer.h"
#include "minipeak/vulkan/Context.h"

#include "shaders/compute_sp_v1.spv.h"
#include "shaders/compute_sp_v2.spv.h"
#include "shaders/compute_sp_v4.spv.h"
#include "shaders/compute_sp_v8.spv.h"
#include "shaders/compute_sp_v16.spv.h"
#include "timeit.h"

int main(int argc, char **argv) {
    uint64_t globalWIs = 256;

    int localSize = 256;
    auto ctx = Context::Inst();

    //uint64_t maxWG = ctx->PhysicalDevice.getProperties().limits.maxComputeWorkGroupInvocations;
    //globalWIs = std::min(globalWIs, maxWG - maxWG % localSize);

    auto buffer = std::make_shared<VBuffer>('f', globalWIs, globalWIs, globalWIs, BufferInfo_t::usage_t::intermediate);
    auto a_buffer = std::make_shared<VBuffer>(BufferInfo_t('f', 1, 1, 1, BufferInfo_t::usage_t::uniform, BufferInfo_t::order_t::CHW));
    float A = -1e-5;
    a_buffer->write(&A);

    struct kernel_info_t {
        std::string name;
        const uint8_t* data;
        size_t length;
    };

#define CREATE_KERNEL(n) kernel_info_t{ #n, n##_spv, sizeof(n##_spv)}
    auto kernels = { CREATE_KERNEL(compute_sp_v1), CREATE_KERNEL(compute_sp_v2), CREATE_KERNEL(compute_sp_v4), CREATE_KERNEL(compute_sp_v8), CREATE_KERNEL(compute_sp_v16) };

    for(auto& k : kernels) {

        auto program = ComputeNode(k.name.c_str(), k.data, k.length, globalWIs / localSize, globalWIs, globalWIs, {buffer, a_buffer});
        program();

        std::vector<uint8_t> d;
        d.resize(globalWIs*globalWIs*globalWIs*sizeof(float));

//        std::string fn = std::string("result-vk-") + k.name.c_str() + ".bin";
//        auto f = fopen(fn.c_str(), "w");
        buffer->read(d.data());
//        fwrite(d.data(), 1, d.size(), f);
//        fclose(f);

        auto checksum = crc32(0, d.data(), d.size());
        printf("%08lx  ", checksum);

        program();

        {
            Timeit t(k.name, 4096  * (double)(globalWIs*globalWIs*globalWIs) / 1e9f, "GFLOPs");
            do {
                program();
            } while(t.under(2));
        }
    }

    return 0;
}
