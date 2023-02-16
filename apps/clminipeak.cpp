#include "string"

#include "minipeak/gpu/BufferInfo.h"
#include "minipeak/gpu/DeviceInfo.h"
#include "minipeak/opencl/Kernel.h"

#include <sstream>

#include "string"
#include "timeit.h"

static std::string source = R"18e3c792b59(
// Avoiding auto-vectorize by using vector-width locked dependent code
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#undef MAD_4
#undef MAD_16
#undef MAD_64

#if dtype == int 
#define mad(a,b,c) ((a)*(b)+(c))
#endif

#define MAD_4(x, y)     x = mad(y, x, y);   y = mad(x, y, x);   x = mad(y, x, y);   y = mad(x, y, x);
#define MAD_16(x, y)    MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);
#define MAD_64(x, y)    MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);

__kernel void compute_sp_v1(__global dtype *ptr, float _A)
{
    dtype x = _A;
    dtype y = (dtype)get_local_id(0);

    for(int i=0; i<128; i++)
    {
        MAD_16(x, y);
    }

    ptr[get_global_id(0)] = y;
}

__kernel void compute_sp_v2(__global dtype *ptr, float _A)
{
    dtype2 x = (dtype2)(_A, (_A+1));
    dtype2 y = (dtype2)get_local_id(0);

    for(int i=0; i<64; i++)
    {
        MAD_16(x, y);
    }

    ptr[get_global_id(0)] = (y.S0) + (y.S1);
}

__kernel void compute_sp_v4(__global dtype *ptr, float _A)
{
    dtype4 x = (dtype4)(_A, (_A+1), (_A+2), (_A+3));
    dtype4 y = (dtype4)get_local_id(0);

    for(int i=0; i<32; i++)
    {
        MAD_16(x, y);
    }

    ptr[get_global_id(0)] = (y.S0) + (y.S1) + (y.S2) + (y.S3);
}

__kernel void compute_sp_v8(__global dtype *ptr, float _A)
{
    dtype8 x = (dtype8)(_A, (_A+1), (_A+2), (_A+3), (_A+4), (_A+5), (_A+6), (_A+7));
    dtype8 y = (dtype8)get_local_id(0);

    for(int i=0; i<16; i++)
    {
        MAD_16(x, y);
    }

    ptr[get_global_id(0)] = (y.S0) + (y.S1) + (y.S2) + (y.S3) + (y.S4) + (y.S5) + (y.S6) + (y.S7);
}

// __kernel void compute_sp_v16(__global dtype *ptr, float _A)
// {
//     dtype16 x = (dtype16)(_A, (_A+1), (_A+2), (_A+3), (_A+4), (_A+5), (_A+6), (_A+7),
//                     (_A+8), (_A+9), (_A+10), (_A+11), (_A+12), (_A+13), (_A+14), (_A+15));
//     dtype16 y = (dtype16)get_local_id(0);

//     for(int i=0; i<8; i++)
//     {
//         MAD_16(x, y);
//     }

//     dtype2 t = (y.S01) + (y.S23) + (y.S45) + (y.S67) + (y.S89) + (y.SAB) + (y.SCD) + (y.SEF);
//     ptr[get_global_id(0)] = t.S0 + t.S1;
// }
)18e3c792b59";

using namespace minipeak::gpu;
DeviceInfo_t get_device(const cl::Device& d) {
    DeviceInfo_t devInfo;

    devInfo.deviceName = d.getInfo<CL_DEVICE_NAME>();
    devInfo.driverVersion = d.getInfo<CL_DRIVER_VERSION>();

    devInfo.numCUs = (uint)d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
    std::vector<size_t> maxWIPerDim;
    maxWIPerDim = d.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    devInfo.maxWGSize = (uint)maxWIPerDim[0];

    // Limiting max work-group size to 256
#define MAX_WG_SIZE 256
    devInfo.maxWGSize = std::min(devInfo.maxWGSize, (uint)MAX_WG_SIZE);

    // FIXME limit max-workgroup size for qualcomm platform to 128
    // Kernel launch fails for workgroup size 256(CL_DEVICE_MAX_WORK_ITEM_SIZES)
    std::string vendor = d.getInfo<CL_DEVICE_VENDOR>();
    if ((vendor.find("QUALCOMM") != std::string::npos) ||
        (vendor.find("qualcomm") != std::string::npos))
    {
        devInfo.maxWGSize = std::min(devInfo.maxWGSize, (uint)128);
    }

    devInfo.maxAllocSize = static_cast<uint64_t>(d.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>());
    devInfo.maxGlobalSize = static_cast<uint64_t>(d.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>());

    devInfo.deviceType = d.getInfo<CL_DEVICE_TYPE>();

    if (devInfo.deviceType & CL_DEVICE_TYPE_CPU)
    {
        devInfo.computeWgsPerCU = 512;
    }
    else
    {
        devInfo.computeWgsPerCU = 2048;
    }

    return devInfo;
}

uint64_t roundToMultipleOf(uint64_t number, uint64_t base, uint64_t maxValue = UINT64_MAX)
{
    uint64_t n = (number > maxValue) ? maxValue : number;
    return (n / base) * base;
}

int main() {
    auto context = OCL::Context::Inst();

    auto devInfo = get_device(context->device[0]);
    uint64_t globalWIs = (devInfo.numCUs) * (devInfo.computeWgsPerCU) * (devInfo.maxWGSize);
    uint64_t t = std::min((globalWIs * sizeof(cl_float)), devInfo.maxAllocSize) / sizeof(cl_float);
    globalWIs = roundToMultipleOf(t, devInfo.maxWGSize);

    auto buffer = OCL::Buffer('f', 1, globalWIs, 1, BufferInfo_t::usage_t::output);

    cl::NDRange globalSize, localSize;
    globalSize = globalWIs;
    localSize = devInfo.maxWGSize;

    for(int i = 0;i < 3;i++) {
      std::cout << "Run size " << globalSize[i] << " " << localSize[i] << std::endl;
    }
    for( std::string dtype : {"half", "float", "double", "int"} ) {
      
      auto kernels = {"compute_sp_v1", "compute_sp_v2", "compute_sp_v4", "compute_sp_v8"};
      for(auto& k : kernels) {

	std::stringstream preamble;
	preamble << "#define dtype " << dtype << std::endl;
	preamble << "#define dtype2 " << dtype << "2" << std::endl;
	preamble << "#define dtype4 " << dtype << "4" << std::endl;
	preamble << "#define dtype8 " << dtype << "8" << std::endl;	
        auto kernel = OCL::Kernel(k, {preamble.str() + source}, {buffer});
        cl_float A = -1e-5;
        kernel.kernel.setArg(1, A);

        context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, globalSize, localSize);
        context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, globalSize, localSize);
        context->queue.finish();

        {
	  Timeit t(dtype + " " + k, 4096  * (double)globalWIs / 1e9f, "GFLOPs");
	  do {
	    context->queue.enqueueNDRangeKernel(kernel.kernel, cl::NullRange, globalSize, localSize);
	    context->queue.finish();
	  } while(t.under(2));
        }
      }
    }
}
