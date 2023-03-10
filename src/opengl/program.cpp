#include <cassert>
#include "minipeak/opengl/program.h"

static void delete_program(const GLuint *s)
{
    if (s && *s) {
        //printf("Delete program %d\n", *s);
        glDeleteProgram(*s);
    }
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

void GLSLProgram::bind(const std::vector<GLSLBuffer>& input_buffers, const std::vector<GLSLBuffer>& output_buffers) {
    this->buffers = input_buffers;
    this->buffers.insert(this->buffers.end(), output_buffers.begin(), output_buffers.end());
    for(auto& b: buffers) {
        assert(b.ptr);
    }
}

void GLSLProgram::sync() {
    GLenum waitReturn = GL_UNSIGNALED;

    while (waitReturn != GL_ALREADY_SIGNALED && waitReturn != GL_CONDITION_SATISFIED)
        waitReturn = glClientWaitSync(_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);
}
void GLSLProgram::dispatch(uint64_t x, uint64_t  y, uint64_t z) {
    use();

    for(size_t i = 0;i < buffers.size();i++) {
        glBindBufferBase(buffers[i].target(), i, *buffers[i]);
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
        if(!buffers[i].info.is_static()) {
            glBindBuffer(GL_ARRAY_BUFFER, *buffers[i]);
            glEnableVertexAttribArray(
                    i); // Attribute indexes were received from calls to glGetAttribLocation, or passed into glBindAttribLocation.
            glVertexAttribPointer(i, buffers[i].info.c, GL_FLOAT, false, 0,
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
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    //glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture( GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

static void delete_shader(const GLuint *s)
{
    if (s && *s) {
        glDeleteShader(*s);
    }
}

gl_ptr compile_shader(const std::string& name, const std::vector<std::string>& source, GLuint PROGRMA_TYPE) {
    gl_ptr shader(new GLuint(0), &delete_shader);

    *shader = glCreateShader(PROGRMA_TYPE);
    std::vector<const GLchar*> buffers;
    std::vector<GLint> lengths;
    std::string preamble = "#version 310 es\n";

    buffers.push_back(preamble.c_str());
    lengths.push_back(preamble.size());

    if(getenv("CR_DEBUG_SHADERS")) {
      printf("// SHADER SOURCE: %s\n", name.c_str());
    }

    for(auto& s : source) {
        buffers.push_back(s.c_str());
        lengths.push_back(s.size());
	if(getenv("CR_DEBUG_SHADERS")) {
	  printf("%s\n", s.c_str());
	}
    }
    
    glShaderSource(*shader, buffers.size(), buffers.data(), lengths.data());
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint il = 0;
        glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &il);
        if (il > 1) {
            std::vector<char> log;
            log.resize(il);

            glGetShaderInfoLog(*shader, il, 0x0, &log[0]);
            for(size_t i = 0;i < buffers.size();i++) {
                fprintf(stderr, "%s\n", buffers[i]);
            }
            fprintf(stderr, "Error compiling shader '%s': \n%s", name.c_str(), &log[0]);
        }
        fprintf(stderr, "cannot compile shader\n");
        shader.reset();
    }

    return shader;
}
