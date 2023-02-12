#pragma once
#include "vector"
#include "array"

#include "buffer.h"
#include "minipeak/gpu/Buffer.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <memory>


typedef std::shared_ptr<GLuint> gl_ptr;
struct GLSLProgram;

struct GLSLBuffer : public GPUBuffer {
    using order_t = BufferInfo_t::order_t;
    gl_ptr ptr = nullptr;

    uint32_t target() const {
        return info.uniform() ? GL_UNIFORM_BUFFER : GL_SHADER_STORAGE_BUFFER;
    }

    std::shared_ptr<GPUBuffer> clone() const override {
        return std::make_shared<GLSLBuffer>(info);
    }

    GLSLBuffer copy() const {
        return GLSLBuffer(info);
    }

    template <typename... Args>
    GLSLBuffer(Args... args) : GLSLBuffer(BufferInfo_t(args...)) {}

    GLSLBuffer() {}
    GLSLBuffer(const BufferInfo_t& info);
    GLSLBuffer(const BufferInfo_t& info, bool persistent, bool bufferStorage, int flags);  
    GLSLBuffer(const GLSLBuffer&) = default;
    GLSLBuffer(GLSLBuffer&&) = default;

    GLSLBuffer& operator=(const GLSLBuffer&) = default;

    std::string glsl_type(int vec = 1) const;
    std::string work_type(int vec = 1) const;  

    void read(void* dst) const override;
    void write(const void* src) override;

    std::string glsl_constants(int vec = 1) const { return glsl_constants("", vec); }
    std::string glsl_constants(const std::string& prefix = "", int vec = 1) const;

    GLuint operator*() const { return *ptr; }

};
