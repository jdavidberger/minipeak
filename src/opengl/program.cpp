#include <cassert>
#include "minipeak/opengl/program.h"
#include "src/opengl/data_access.glsl.h"

static void delete_program(const GLuint *s)
{
    if (s && *s) {
        //printf("Delete program %d\n", *s);
        glDeleteProgram(*s);
    }
}

GLSLProgram::GLSLProgram(const std::vector<GLSLShaderDef> &shaders) {
    ptr = ShaderDB::g().Compile(shaders);
}

static gl_ptr create_program(const std::vector<gl_ptr>& shaders) {
    auto program = gl_ptr(new GLuint(glCreateProgram()), delete_program);

    for(auto& shader : shaders) {
        assert(shader);
        if(shader == 0){
            return 0;
        }
        glAttachShader(*program, *shader);
    }

    glLinkProgram(*program);

    GLint linked = 0;
    glGetProgramiv(*program, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint il = 0;
        glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &il);
        if (il > 1) {
            std::vector<char> log;
            log.resize(il);

            glGetProgramInfoLog(*program, il, 0x0, &log[0]);
            fprintf(stderr, "Error linking: \n%s", &log[0]);
        }
        fprintf(stderr, "cannot link program");
        program.reset();
    }

    return program;
}

void GLSLProgram::use() {
    //printf("Use %d\n", *ptr);
    glUseProgram(*ptr);
}

GLSLProgram::GLSLProgram(const std::vector<gl_ptr> &shaders) : shaders(shaders) {
    ptr = create_program(shaders);
}

void GLSLProgram::bind(const std::vector<GLSLBuffer>& input_buffers, bool allow_null) {
    this->buffers = input_buffers;

    if(!allow_null) {
        for (auto &b: buffers) {
            assert(b.ptr);
        }
    }
}

void GLSLProgram::sync() {
    GLenum waitReturn = GL_UNSIGNALED;

    while (waitReturn != GL_ALREADY_SIGNALED && waitReturn != GL_CONDITION_SATISFIED)
        waitReturn = glClientWaitSync(_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
}
void GLSLProgram::dispatch(uint64_t x, uint64_t  y, uint64_t z) {
    use();

    for(size_t i = 0;i < buffers.size();i++) {
        BindBuffer(i);
    }
    //glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, ssbos.size(), ssbos.data());

    //printf("Dispatch %d %d %d\n", x, y, z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glDispatchCompute(x, y, z);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glDeleteSync(_sync);
    _sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    glUseProgram(0);
}

void GLSLProgram::set_uniform(int binding, const std::array<float, 16>& value) {
    use();
    glUniformMatrix4fv(binding, 1, 0, value.data());
    glUseProgram(0);
}

void GLSLProgram::set_uniform(int binding, bool value) {
    use();
    glUniform1i(binding, value);
    glUseProgram(0);
}

void GLSLProgram::set_uniform(int binding, GLuint value) {
    use();
    glUniform1ui(binding, value);
    glUseProgram(0);
}

void GLSLProgram::set_uniform(int binding, GLint value) {
    use();
    glUniform1i(binding, value);
    glUseProgram(0);
}

void GLSLProgram::set_uniform(int binding, float value) {
    use();
    glUniform1f(binding, value);
    glUseProgram(0);
}

void GLSLProgram::draw(GLenum mode, GLsizei count) {
    if(ptr == 0) {
        return;
    }

    use();

    for(size_t i = 0;i < buffers.size();i++) {
        if(buffers[i].ptr == 0) continue;

        if(!buffers[i].info.is_static()) {
            glBindBuffer(GL_ARRAY_BUFFER, *buffers[i]);
            glEnableVertexAttribArray(
                    i); // Attribute indexes were received from calls to glGetAttribLocation, or passed into glBindAttribLocation.
            glVertexAttribPointer(i, buffers[i].info.order == BufferInfo_t::order_t::HWC ? buffers[i].info.c : 1, GL_FLOAT, false, 0,
                                  nullptr); // texcoords_data is a float*, 2 per vertex, representing UV coordinates.
        } else {
            glBindBufferBase(buffers[i].target(), i, *buffers[i]);
        }
    }

    if(texture_ptr) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *texture_ptr);
    }

    glDrawArrays(mode, 0, count);

    for(size_t i = 0;i < buffers.size();i++) {
        if(!buffers[i].info.is_static()) {
            glDisableVertexAttribArray(i);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteSync(_sync);
    _sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    glUseProgram(0);
}

void GLSLProgram::draw() {
    if(ptr == 0) {
        return;
    }

    use();

    for(size_t i = 0;i < buffers.size();i++) {
        if(buffers[i].ptr)
            glBindBufferBase(buffers[i].target(), i, *buffers[i]);
    }

    if(VAO == 0) {
        glGenVertexArrays(1, &VAO);
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    draw(GL_TRIANGLES, 6);

    glDeleteSync(_sync);
    _sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    glUseProgram(0);
}
void free_texture(const GLuint* tex) {
    if(tex) {
        glDeleteTextures(1, tex);
    }
}

void GLSLProgram::write_texture(int width, int height, const void *data) {
    if(ptr == nullptr) return;
    use();

    width = std::max(width, 1);
    height = std::max(height, 1);
    if(texture_width != width || texture_height != height) {
        texture_width = width;
        texture_height = height;
        texture_ptr.reset();
    }

    if(texture_ptr == nullptr) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        texture_ptr = gl_ptr(new GLuint (texture), free_texture);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *texture_ptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, width, height);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *texture_ptr);
    if(data) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
        uint32_t empty = 0;
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &empty);
    }
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    //glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture( GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void GLSLProgram::BindBuffer(int buffer_idx) {
//    int idx = glGetProgramResourceIndex(*ptr, GL_SHADER_STORAGE_BLOCK, buffers[buffer_idx].info.name.c_str());
//    if(idx >= 0) {
//        glShaderStorageBlockBinding(*ptr, idx, buffer_idx);
//    }
    if(buffers[buffer_idx]) {
        glBindBufferBase(buffers[buffer_idx].target(), buffer_idx, *buffers[buffer_idx]);
    }
}

static void delete_shader(const GLuint *s)
{
    if (s && *s) {
        glDeleteShader(*s);
    }
}

gl_ptr compile_shader(const std::string& name, const std::vector<std::string>& _source, GLuint PROGRMA_TYPE) {
    gl_ptr shader(new GLuint(0), &delete_shader);

    *shader = glCreateShader(PROGRMA_TYPE);

    std::string preamble = "#version 310 es\n";

    std::vector<std::string> source = { preamble, data_access_glsl };
    if(getenv("MINIPEAK_DEBUG_SHADERS")) {
      printf("// SHADER SOURCE: %s\n", name.c_str());
    }

    auto data_access = std::string(data_access_glsl);

    for(auto& s : _source) {
        if(s.find("#include \"") == 0) {
            auto file = std::string(s.c_str() + strlen("#include \""),
                                    s.c_str() + s.rfind("\""));
            auto lu = ShaderDB::g().getSource(file);
            source.push_back(lu);
        } else {
            source.push_back(s);
        }
    }

    std::vector<const GLchar*> buffers;
    std::vector<GLint> lengths;

    for(auto& s : source) {
      if(getenv("MINIPEAK_DEBUG_SHADERS")) {
          printf("%s\n", s.c_str());
      }
      buffers.push_back(s.data());
      lengths.push_back(s.length());
    }

    glShaderSource(*shader, buffers.size(), buffers.data(), lengths.data());
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint il = 0;
        glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &il);

        for(size_t i = 0;i < buffers.size();i++) {
            fprintf(stderr, "%s\n", buffers[i]);
        }

        if (il > 0) {
            std::vector<char> log;
            log.resize(il);

            glGetShaderInfoLog(*shader, il, 0x0, &log[0]);
            fprintf(stderr, "Error compiling shader '%s': \n%s", name.c_str(), &log[0]);
        }
        fprintf(stderr, "cannot compile shader\n");
        shader.reset();
    }

    return shader;
}
