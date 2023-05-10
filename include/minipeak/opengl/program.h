#pragma once

#include "iostream"
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
    int texture_width = 0, texture_height = 0;
    gl_ptr texture_ptr;

    GLuint VAO = 0;
    std::vector<gl_ptr> shaders;
    std::vector<GLSLBuffer> buffers;

    GLsync _sync = 0;

    GLSLProgram() = default;
    GLSLProgram(const std::vector<gl_ptr>& shaders);
    void bind(const std::vector<GLSLBuffer>& input_buffers, bool allow_null = false);
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
        use();
        auto uniformLoc = glGetUniformLocation(*this->ptr, binding.c_str());
        if(uniformLoc == -1) {
            std::cerr << "Can not find uniform for " << binding << std::endl;
        }
        set_uniform(uniformLoc, value);
        glUseProgram(0);
    }
    void draw(GLenum mode, GLsizei count);
    void draw();
    void dispatch(uint64_t x = 1, uint64_t y = 1, uint64_t z = 1);
    void sync();

    void write_texture(int width, int height, const void* data);
};

gl_ptr compile_shader(const std::string& name, const std::vector<std::string>& source, GLuint PROGRMA_TYPE = GL_COMPUTE_SHADER);
