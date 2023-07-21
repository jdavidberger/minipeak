#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <string.h>

#include <iostream>
#include <chrono>
#include "timeit.h"
#include "zlib.h"
#include <sstream>

#include <minipeak/gpu/BufferInfo.h>
#include <minipeak/opengl/buffer.h>
#include <minipeak/opengl/program.h>
#include <vector>

bool ignore_debug = false;
void DEBUGPROC(GLenum source,
               GLenum type,
               GLuint id,
               GLenum severity,
               GLsizei length,
               const GLchar *message,
               const void *userParam) {
    if(ignore_debug) return;

    const char* severity_cstr = "Unknown";
    switch(severity) {
        case GL_DEBUG_SEVERITY_HIGH: severity_cstr = "High"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: severity_cstr = "Medium"; break;
        case GL_DEBUG_SEVERITY_LOW: severity_cstr = "Low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severity_cstr = "Notification"; break;
    }

    if(severity != GL_DEBUG_SEVERITY_HIGH) {
        printf("gl error %24s %d %s\n", severity_cstr, id, message);
    } else {
        fprintf(stderr, "gl error %d %s\n", id, message);
    }
}

void error_cb(int error_code, const char* description) {
    printf("glfw error %d %s\n", error_code, description);
}

static std::string header = R"18e3c792b59(
#line 60

layout(std430, binding = 0) restrict readonly buffer inbuffer {
    float inptr[];
};

layout(std430, binding = 1) restrict writeonly buffer outbuffer {
    float ptr[];
};

#define SCALE 1e-10

layout(local_size_x = LOCAL_SIZE_X) in;
layout(location = 1) uniform uint access_stride;
)18e3c792b59";

static std::string memread = R"18e3c792b59(
#line 64
void memread()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    float y = float(float(id) * SCALE);

    for(uint i = 0u;i < width*height;i++) {
       y *= inptr[(i * access_stride + id) % (width*height)];
    }

    ptr[id] = y;
}
)18e3c792b59";

int main(int argc, char **argv) {
    uint64_t localSize = 256;
    uint64_t globalMult = 32;
    if(argc > 1) {
        localSize = atoll(argv[1]);
    }
    if(argc > 2) {
      globalMult = atoll(argv[2]);
    }
    glfwSetErrorCallback(error_cb);
    if (!glfwInit()) {
        const char *error_msg = 0;
        glfwGetError(&error_msg);
        printf("Could not init glfw %s", error_msg);
        return -1;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *offscreen_context = glfwCreateWindow(640, 480, "", NULL, NULL);

    glfwMakeContextCurrent(offscreen_context);

    GLint major = 0, minor = 0;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    printf("GL version: %d.%d\n", major, minor);
    printf("GL Shading language version %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Vendor %s\n", glGetString(GL_VENDOR));
    printf("Renderer %s %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

    int workGroupSizes[3] = { 0 };
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSizes[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workGroupSizes[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workGroupSizes[2]);
    int workGroupCounts[3] = { 0 };
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workGroupCounts[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workGroupCounts[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workGroupCounts[2]);

    GLint maxInvocations = 0;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);
    printf("Invocations size %d\n", maxInvocations);
    printf("Work group size  %d %d %d\n", workGroupSizes[0], workGroupSizes[1], workGroupSizes[2]);
    printf("Work group count %d %d %d\n", workGroupCounts[0], workGroupCounts[1], workGroupCounts[2]);

    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);

    int flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(DEBUGPROC, 0);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);
    }

    uint64_t workGroupCount_target = std::min(workGroupCounts[0] / 8, 256*256*32);
    uint64_t globalWIs = globalMult * localSize;//(uint64_t)(workGroupCount_target) * localSize;
    printf("Running size %lu / %d\n", globalWIs, localSize);

    auto out_buffer = GLSLBuffer('f', globalWIs, 1, 1, BufferInfo_t::usage_t::intermediate);

    auto memaccess = std::vector<std::pair<std::string, int>> {
            {"linear", 1},
            {"random", 1223},
    };

    GLint size;
#define SHOW_STORAGE_SIZE(x) \
    glGetIntegerv(x, &size);		\
    printf("%-32s %.2fKb\n", #x, size / 1024.);

    SHOW_STORAGE_SIZE(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
    SHOW_STORAGE_SIZE(GL_MAX_TEXTURE_BUFFER_SIZE);
    SHOW_STORAGE_SIZE(GL_MAX_TEXTURE_SIZE);    
    SHOW_STORAGE_SIZE(GL_MAX_UNIFORM_BLOCK_SIZE);
    SHOW_STORAGE_SIZE(GL_UNIFORM_BUFFER_SIZE);

    glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &size);
    printf("%-32s %d\n", "GL_MAX_COMPUTE_UNIFORM_BLOCKS", size);
    
#define GPU_FLAG(x) { #x, false, false, x}
    auto usages = std::vector<std::tuple<std::string, bool, bool, int>> {
      {"intermediate-buffer-persistent", true, true, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT},
      {"intermediate-buffer", false, true, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT},
      {"intermediate", false, false, GL_DYNAMIC_COPY},
      {"input", true, true, GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT},
      {"output", true, true, GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT},
      {"static", false, false, GL_STATIC_DRAW},
      GPU_FLAG(GL_STREAM_DRAW),
      GPU_FLAG(GL_STREAM_READ),
      GPU_FLAG(GL_STREAM_COPY),
      GPU_FLAG(GL_STATIC_READ),
      GPU_FLAG(GL_STATIC_COPY),
      GPU_FLAG(GL_DYNAMIC_DRAW),
      GPU_FLAG(GL_DYNAMIC_READ),
      GPU_FLAG(GL_DYNAMIC_COPY),
    };

    int width = 512, height = 512;

    std::vector<uint8_t> d, d1;
    d.resize(width*height*4);
    d1.resize(width*height*4);
    for(auto&v : d) {
      v = 0xce;
    }

    {
      Timeit t("Memcpy baseline", width * height * 4  / 1024. / 1024., "MBs");
      do {
	memcpy(d1.data(), d.data(), d.size());
      } while (t.under(1));
    }

    
    for(auto& k : usages) {
      auto name = std::get<0>(k);
      auto buffer = GLSLBuffer(BufferInfo_t('f', 1, width, height), std::get<1>(k), std::get<2>(k), std::get<3>(k));
        buffer.write_vector(d);
        ignore_debug=true;
        printf("%s:\n", name.c_str());
        {
            Timeit t("\tread", width * height * 4  / 1024. / 1024., "MBs");
            do {
                buffer.read(d.data());
            } while (t.under(1));
        }

        {
            Timeit t("\twrite", width * height * 4 / 1024. / 1024., "MBs");
            do {
                buffer.write_vector(d);
            } while (t.under(1));
        }
        ignore_debug=false;
        for(auto& m : memaccess) {

            std::stringstream ss;
            ss << "#define LOCAL_SIZE_X " << localSize << std::endl;
            auto program = GLSLProgram({
                                               compile_shader(name, {ss.str(), buffer.glsl_constants(), header, memread,
                                                                        "void main() {memread();}"})
                                       });
            program.bind({buffer, out_buffer});

            program.set_uniform(1, (GLuint)m.second);

            program.dispatch(globalWIs / localSize, 1, 1);
            program.sync();

            for (int i = 0; i < 3; i++) {
                program.dispatch(globalWIs / localSize, 1, 1);
                program.sync();
            }

            {
                Timeit t("\t" + m.first, width * height * 4 * (double) (globalWIs) / 1024. / 1024., "MBs");
                do {
                    program.dispatch(globalWIs / localSize, 1, 1);
                    program.sync();
                } while (t.under(1));
            }
        }
    }

    return 0;
}
