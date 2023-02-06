#pragma once

#include "sstream"
#include <map>

#include "minipeak/opengl/buffer.h"
#include <memory>
#include <GL/glu.h>
#include <GL/gl.h>
#include "array"
#include "vector"

struct GLSLProgram {
    gl_ptr ptr;
    gl_ptr texture_ptr;

    GLuint VAO = 0;
    std::vector<gl_ptr> shaders;
    std::vector<GLSLBuffer> buffers;

    GLsync _sync = 0;

    GLSLProgram() = default;
    GLSLProgram(const std::vector<gl_ptr>& shaders);
    void bind(const std::vector<GLSLBuffer>& input_buffers, const std::vector<GLSLBuffer>& output_buffers = {});
    void use();

    operator bool() const { return ptr.operator bool(); }
    void set_uniform(int binding, float value);
    void set_uniform(int binding, bool value);
    void set_uniform(int binding, const std::array<float, 16>& value);
    void set_uniform(int binding, GLint value);
    void set_uniform(int binding, GLuint value);
    template <typename T>
    void set_uniform(const std::string& binding, T value) {
        if(ptr == nullptr) return;
        set_uniform(glGetUniformLocation(*this->ptr, binding.c_str()), value);
    }
    void draw(GLenum mode, GLsizei count);
    void draw();
    void dispatch(int x = 1, int y = 1, int z = 1);
    void sync();

    void write_texture(int width, int height, const void* data);
};

gl_ptr compile_shader(const std::string& name, const std::vector<std::string>& source, GLuint PROGRMA_TYPE = GL_COMPUTE_SHADER);
