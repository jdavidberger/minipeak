#include <iostream>
#include "minipeak/opencl/Kernel.h"

OCL::Kernel::Kernel(const std::string &name, const std::vector<std::string> &sources,
                    const std::vector<Buffer> &buffers) : context(Context::Inst()) {
    this->name = name;
    program = cl::Program (context->context, sources);

    try {
        program.build({context->device[0]});
    } catch (const cl::Error& error) {
      std::cerr << "Encountered " << OCL::errstr(error.err()) << " during " << error.what() << " for " << name << std::endl;
        std::cerr
                << "OpenCL compilation error" << std::endl
                << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(context->device[0])
                << std::endl;
	for(auto& s : sources) {
	  fprintf(stderr, "%s\n", s.c_str());
	}
	
	throw error;
    }

    try {
      kernel = cl::Kernel(program, name.c_str());
      for(size_t i = 0;i < buffers.size();i++) {
        kernel.setArg(i, *buffers[i].b);
      }
    }
    catch (const cl::Error& error) {
      std::cerr << "Encountered " << OCL::errstr(error.err()) << " during " << error.what() << " for " << name << std::endl;
      throw error;
    }
}
