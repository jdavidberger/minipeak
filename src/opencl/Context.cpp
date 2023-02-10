#include "minipeak/opencl/Context.h"
#include <stdio.h>
#include <iostream>

namespace OCL {
    std::weak_ptr<Context> Context::g;

    Context::Context() {
        std::vector<cl::Platform> platform;
        cl::Platform::get(&platform);

	std::cout << platform.size() << " platforms found " << std::endl;
        for(auto p = platform.begin(); p != platform.end(); p++) {
            std::vector<cl::Device> pldev;
            std::cout << p->getInfo<CL_PLATFORM_NAME>() << std::endl;
            try {
                p->getDevices(CL_DEVICE_TYPE_ALL, &pldev);

                for(auto d = pldev.begin(); d != pldev.end(); d++) {
		  if (!d->getInfo<CL_DEVICE_AVAILABLE>()) {
		    std::cout << d->getInfo<CL_DEVICE_VERSION>() << " not available" << std::endl;
		    continue;
		  }

                    std::string ext = d->getInfo<CL_DEVICE_EXTENSIONS>();

                    std::cout << d->getInfo<CL_DEVICE_NAME>() << std::endl;
                    auto is_gpu = d->getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU;
                    if(is_gpu)
                        device.insert(device.begin(), *d);
                    else
                        device.push_back(*d);
                }
            } catch(...) {
	      std::cout << "Error; clearing devices\n" << std::endl;
            }
        }

        std::cout << device[0].getInfo<CL_DEVICE_NAME>() << " choosen" << std::endl;
        context = cl::Context(device[0]);
        queue = cl::CommandQueue(context, device[0]);
    }

    std::shared_ptr<Context> Context::Inst() {
        if (auto ptr = g.lock()) {
            return ptr;
        }
        auto rtn = std::shared_ptr<Context>(new Context());
        g = rtn;
        return rtn;
    }

    std::shared_ptr<cl::Buffer> Context::AllocBuffer(const BufferInfo_t &info, cl_mem_flags flags) {
        return std::make_shared<cl::Buffer>(context, flags, info.buffer_size());
    }

    std::shared_ptr<cl::Buffer> Context::AllocBuffer(const BufferInfo_t &info) {
        return AllocBuffer(info, GetFlagFromUsage(info.usage));
    }

    Context::~Context() {

    }

    cl_mem_flags Context::GetFlagFromUsage(BufferInfo_t::usage_t usage) {
        cl_mem_flags flags = 0;
        if((usage & BufferInfo_t::usage_t::host_available) == BufferInfo_t::usage_t::host_available) {
            flags |= CL_MEM_ALLOC_HOST_PTR;
        }
        if((usage & BufferInfo_t::usage_t::input) == BufferInfo_t::usage_t::input) {
            flags |= CL_MEM_HOST_WRITE_ONLY | CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR;
        }
        if((usage & BufferInfo_t::usage_t::output) == BufferInfo_t::usage_t::output) {
            flags |= CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_ALLOC_HOST_PTR;
        }
        if((usage & BufferInfo_t::usage_t::uniform) == BufferInfo_t::usage_t::uniform) {
            flags |= CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY;
        }
        if (usage == BufferInfo_t::usage_t::intermediate) {
            flags |= CL_MEM_READ_WRITE;
        }
        return flags;
    }

#define CaseReturnString(x) case x: return #x;

const char *errstr(cl_int err)
{
    switch (err)
    {
        CaseReturnString(CL_SUCCESS                        )                                  
        CaseReturnString(CL_DEVICE_NOT_FOUND               )
        CaseReturnString(CL_DEVICE_NOT_AVAILABLE           )
        CaseReturnString(CL_COMPILER_NOT_AVAILABLE         ) 
        CaseReturnString(CL_MEM_OBJECT_ALLOCATION_FAILURE  )
        CaseReturnString(CL_OUT_OF_RESOURCES               )
        CaseReturnString(CL_OUT_OF_HOST_MEMORY             )
        CaseReturnString(CL_PROFILING_INFO_NOT_AVAILABLE   )
        CaseReturnString(CL_MEM_COPY_OVERLAP               )
        CaseReturnString(CL_IMAGE_FORMAT_MISMATCH          )
        CaseReturnString(CL_IMAGE_FORMAT_NOT_SUPPORTED     )
        CaseReturnString(CL_BUILD_PROGRAM_FAILURE          )
        CaseReturnString(CL_MAP_FAILURE                    )
        CaseReturnString(CL_MISALIGNED_SUB_BUFFER_OFFSET   )
        CaseReturnString(CL_COMPILE_PROGRAM_FAILURE        )
        CaseReturnString(CL_LINKER_NOT_AVAILABLE           )
        CaseReturnString(CL_LINK_PROGRAM_FAILURE           )
        CaseReturnString(CL_DEVICE_PARTITION_FAILED        )
        CaseReturnString(CL_KERNEL_ARG_INFO_NOT_AVAILABLE  )
        CaseReturnString(CL_INVALID_VALUE                  )
        CaseReturnString(CL_INVALID_DEVICE_TYPE            )
        CaseReturnString(CL_INVALID_PLATFORM               )
        CaseReturnString(CL_INVALID_DEVICE                 )
        CaseReturnString(CL_INVALID_CONTEXT                )
        CaseReturnString(CL_INVALID_QUEUE_PROPERTIES       )
        CaseReturnString(CL_INVALID_COMMAND_QUEUE          )
        CaseReturnString(CL_INVALID_HOST_PTR               )
        CaseReturnString(CL_INVALID_MEM_OBJECT             )
        CaseReturnString(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
        CaseReturnString(CL_INVALID_IMAGE_SIZE             )
        CaseReturnString(CL_INVALID_SAMPLER                )
        CaseReturnString(CL_INVALID_BINARY                 )
        CaseReturnString(CL_INVALID_BUILD_OPTIONS          )
        CaseReturnString(CL_INVALID_PROGRAM                )
        CaseReturnString(CL_INVALID_PROGRAM_EXECUTABLE     )
        CaseReturnString(CL_INVALID_KERNEL_NAME            )
        CaseReturnString(CL_INVALID_KERNEL_DEFINITION      )
        CaseReturnString(CL_INVALID_KERNEL                 )
        CaseReturnString(CL_INVALID_ARG_INDEX              )
        CaseReturnString(CL_INVALID_ARG_VALUE              )
        CaseReturnString(CL_INVALID_ARG_SIZE               )
        CaseReturnString(CL_INVALID_KERNEL_ARGS            )
        CaseReturnString(CL_INVALID_WORK_DIMENSION         )
        CaseReturnString(CL_INVALID_WORK_GROUP_SIZE        )
        CaseReturnString(CL_INVALID_WORK_ITEM_SIZE         )
        CaseReturnString(CL_INVALID_GLOBAL_OFFSET          )
        CaseReturnString(CL_INVALID_EVENT_WAIT_LIST        )
        CaseReturnString(CL_INVALID_EVENT                  )
        CaseReturnString(CL_INVALID_OPERATION              )
        CaseReturnString(CL_INVALID_GL_OBJECT              )
        CaseReturnString(CL_INVALID_BUFFER_SIZE            )
        CaseReturnString(CL_INVALID_MIP_LEVEL              )
        CaseReturnString(CL_INVALID_GLOBAL_WORK_SIZE       )
        CaseReturnString(CL_INVALID_PROPERTY               )
        CaseReturnString(CL_INVALID_IMAGE_DESCRIPTOR       )
        CaseReturnString(CL_INVALID_COMPILER_OPTIONS       )
        CaseReturnString(CL_INVALID_LINKER_OPTIONS         )
        CaseReturnString(CL_INVALID_DEVICE_PARTITION_COUNT )
        default: return "Unknown OpenCL error code";
    }
}
}
