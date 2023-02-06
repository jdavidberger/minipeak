#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include <iostream>
#include <map>

#include "zlib.h"
#include "minipeak/gpu/BufferInfo.h"
#include "minipeak/opengl/buffer.h"
#include "minipeak/opengl/program.h"

#include "timeit.h"

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
#line 62
// Avoiding auto-vectorize by using vector-width locked dependent code

layout(local_size_x = 256) in;

#undef MAD_4
#undef MAD_16
#undef MAD_64

#define mad(a,b,c) (a*b+c)
#define MAD_4(x, y)     x = mad(y, x, y);   y = mad(x, y, x);   x = mad(y, x, y);   y = mad(x, y, x);
#define MAD_16(x, y)    MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);
#define MAD_64(x, y)    MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);

struct vec8 {
    vec4 d0, d1;
};
#define VEC8(x0,x1,x2,x3,x4,x5,x6,x7) vec8(vec4(x0,x1,x2,x3), vec4(x4,x5,x6,x7))
#define VEC8_S(x) vec8(vec4(x,x,x,x), vec4(x,x,x,x))

#define VEC8_ADD(a, b) (vec8(a.d0 + b.d0, a.d1 + b.d1))
#define VEC8_MUL(a, b) (vec8(a.d0 * b.d0, a.d1 * b.d1))

struct vec16 {
    vec8 d0,d1;
};

#define VEC16(x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15) vec16(VEC8(x0,x1,x2,x3,x4,x5,x6,x7), VEC8(x8,x9,x10,x11,x12,x13,x14,x15))
#define VEC16_S(x) vec16(VEC8_S(x), VEC8_S(x));

#define VEC16_ADD(a, b) (vec16(VEC8_ADD(a.d0, b.d0), VEC8_ADD(a.d1, b.d1)))
#define VEC16_MUL(a, b) (vec16(VEC8_MUL(a.d0, b.d0), VEC8_MUL(a.d1, b.d1)))

#define mad8(a,b,c) (VEC8_ADD(VEC8_MUL(a,b),c))
#define mad16(a,b,c) (VEC16_ADD(VEC16_MUL(a,b),c))

layout(location = 1) uniform float _A;

#define SCALE 1e-10

layout(std430, binding = 0) restrict writeonly buffer outbuffer {
    float ptr[];
};

)18e3c792b59";

static std::string compute_sp_v1 = R"18e3c792b59(
void compute_sp_v1()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    float x = _A;
    float y = float(id) * SCALE;

    for(int i=0; i<128; i++)
    {
        MAD_16(x, y);
    }

    ptr[id] = y;
}
)18e3c792b59";

static std::string compute_sp_v2 = R"18e3c792b59(
void compute_sp_v2()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    vec2 x = vec2(_A, (_A+1.0f));
    vec2 y = vec2(id, id) * SCALE;

    for(int i=0; i<64; i++)
    {
        MAD_16(x, y);
    }

    ptr[id] = (y.x) + (y.y);
}
)18e3c792b59";

static std::string compute_sp_v4 = R"18e3c792b59(
void compute_sp_v4()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    vec4 x = vec4(_A, (_A+1.0f), (_A+2.0f), (_A+3.0f));
    vec4 y = vec4(id, id, id, id) * SCALE;

    for(int i=0; i<32; i++)
    {
        MAD_16(x, y);
    }

    ptr[id] = (y.x) + (y.y) + (y.z) + (y.w);
}
)18e3c792b59";

static std::string compute_sp_v8 = R"18e3c792b59(
void compute_sp_v8()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    vec8 x = VEC8(_A, (_A+1.0f), (_A+2.0f), (_A+3.0f), (_A+4.0f), (_A+5.0f), (_A+6.0f), (_A+7.0f));
    vec8 y = VEC8_S(float(id) * SCALE);


#undef mad
#define mad mad8
    for(int i=0; i<16; i++)
    {
        MAD_16(x, y);
    }

    vec4 s = y.d0 + y.d1;
    vec2 t = s.xy + s.zw;
    ptr[id] = t.x + t.y;
}
)18e3c792b59";

static std::string compute_sp_v16 = R"18e3c792b59(
void compute_sp_v16()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    vec16 x = VEC16(_A, (_A+1.0f), (_A+2.0f), (_A+3.0f), (_A+4.0f), (_A+5.0f), (_A+6.0f), (_A+7.0f),
    (_A+8.0f), (_A+9.0f), (_A+10.0f), (_A+11.0f), (_A+12.0f), (_A+13.0f), (_A+14.0f), (_A+15.0f));
    vec16 y = VEC16_S(float(id) * SCALE);

#undef mad
#define mad mad16
    for(int i=0; i<8; i++)
    {
        MAD_16(x, y);
    }

    vec8 u = VEC8_ADD(y.d0, y.d1);
    vec4 s = u.d0 + u.d1;
    vec2 t = s.xy + s.zw;
    ptr[id] = t.x + t.y;
}
)18e3c792b59";

int main(int argc, char **argv) {
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    uint64_t globalWIs = 256 * 256 * 256;
    
    int localSize = 256;
    auto buffer = GLSLBuffer('f', globalWIs, 1, 1, BufferInfo_t::usage_t::intermediate);
    std::vector<uint8_t> d;
    d.resize(globalWIs*sizeof(float));
    for(auto&v : d) {
        v = 0xce;
    }
    buffer.write(d.data());

    auto sources = std::map<std::string, std::string> {
            {"compute_sp_v1", compute_sp_v1},
            {"compute_sp_v2", compute_sp_v2},
            {"compute_sp_v4", compute_sp_v4},
            {"compute_sp_v8", compute_sp_v8},
            {"compute_sp_v16", compute_sp_v16},
    };

    for(auto& k : sources) {

        std::stringstream ss;
        ss << "#define KERNEL " << k.first << std::endl;

        auto program = GLSLProgram({
                                      compile_shader(k.first, {ss.str(), header, k.second, "void main() {" + k.first +"();}"})
                              });
        program.bind({buffer});

        float A = -1e-5;
        program.set_uniform(1, A);

        program.dispatch(globalWIs / localSize, 1, 1);
        program.sync();
        ignore_debug=1;
        buffer.read(d.data());
        ignore_debug=0;
//        std::string fn = std::string("result-gl-") + k + ".bin";
//        auto f = fopen(fn.c_str(), "w");
//        fwrite(d.data(), d.size(), 1, f);
//        fclose(f);

        auto checksum = crc32(0, d.data(), d.size());
        printf("%08lx  ", checksum);

        for(int i = 0;i < 3;i++) {
            program.dispatch(globalWIs / localSize, 1, 1);
            program.sync();
        }

        {
            Timeit t(k.first, 4096  * (double)(globalWIs) / 1e9f, "GFLOPs");
            do {
                program.dispatch(globalWIs / localSize, 1, 1);
                program.sync();
            } while(t.under(2));
        }
    }

    return 0;
}
