#include "minipeak/opengl/shaderdb.h"
#include "minipeak/opengl/buffer.h"
#include "minipeak/opengl/program.h"
#include <openssl/md5.h>
#include <GL/gl.h>

static void delete_shader(const GLuint *s)
{
    if (s && *s) {
        glDeleteShader(*s);
    }
}

static void delete_program(const GLuint *s)
{
    if (s && *s) {
        //printf("Delete program %d\n", *s);
        glDeleteProgram(*s);
    }
}

gl_ptr ShaderDB::Compile(const std::vector<GLSLShaderDef> &shaders) {
    auto program = gl_ptr(new GLuint(glCreateProgram()), delete_program);

    auto checksum = Checksum(shaders);
    auto binary_file = checksum + ".program.bin";

    GLint formats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
    GLenum binaryFormats[formats];
    glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, reinterpret_cast<GLint *>(binaryFormats));

    bool has_binary = false;
    auto it = binaries.find(checksum);

    if(it != binaries.end()) {
        auto& binary = it->second;
        glProgramBinary(*program, binaryFormats[0], binary.data(), binary.size());
        has_binary = true;
    } else if(FILE* fp = fopen(binary_file.c_str(), "rb")) {
        fseek(fp, 0, SEEK_END);
        GLint len = (GLint)ftell(fp);
        std::vector<uint8_t> program;
        program.resize(len);
        fseek(fp, 0, SEEK_SET);
        fread(program.data(), len, 1, fp);
        fclose(fp);

        Register(checksum, program);
        return Compile(shaders);
    } else {
            for(auto& shader : shaders) {
                glAttachShader(*program, *shader.Compile());
            }

            glLinkProgram(*program);
    }

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

    if(!has_binary) {
        GLint len = 0;
        glGetProgramiv(*program, GL_PROGRAM_BINARY_LENGTH, &len);
        std::vector<uint8_t> binary;
        binary.resize(len + 1);
        glGetProgramBinary(*program, len, NULL, (GLenum *) binaryFormats, binary.data());

        FILE* fp = fopen(binary_file.c_str(), "wb");
        fwrite(binary.data(), len, 1, fp);
        fclose(fp);
    }

    return program;
}

std::string ShaderDB::Checksum(const std::vector<GLSLShaderDef> &shaders) {
    MD5_CTX c;
    unsigned char digest[16];

    MD5_Init(&c);

    auto renderer = std::string((char*)glGetString(GL_RENDERER));
    MD5_Update(&c, renderer.data(), renderer.size());

    for(auto& shader : shaders) {
        MD5_Update(&c, &shader.PROGRAM_TYPE, sizeof(shader.PROGRAM_TYPE));
        for (auto &s: shader.source) {
            MD5_Update(&c, s.c_str(), s.length());
        }
        MD5_Update(&c, "\0", 1);
    }

    MD5_Final(digest, &c);

    std::string hash;
    hash.resize(32);
    for (int n = 0; n < 16; ++n) {
        snprintf(&(hash[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return hash;
}

ShaderDB& ShaderDB::g() {
    static ShaderDB* _g = new ShaderDB();
    return *_g;
}

void ShaderDB::Register(const std::string &name, const std::vector<uint8_t> &src) {
    binaries[name] = src;
}

const std::string &ShaderDB::getSource(const std::string &name) const {
    auto it = sources.find(name);
    if(it == sources.end()) {
        throw std::runtime_error("Could not find requested source file '" + name + "'");
    }
    return it->second;
}

void ShaderDB::Register(const std::string &name, const std::string &src) {
    sources[name] = src;
}

gl_ptr GLSLShaderDef::Compile() const {
    if(compiled_shader == nullptr) {
        compiled_shader = compile_shader(name, source, PROGRAM_TYPE);
    }
    return compiled_shader;
}

GLSLShaderDef::GLSLShaderDef(const std::string &name, const std::vector<std::string> &source, GLuint programType)
        : name(name), source(source), PROGRAM_TYPE(programType) {}
