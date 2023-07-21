#pragma once

#include "memory"
#include "vector"
#include "map"
#include "buffer.h"

struct GLSLShaderDef {
    std::string name;
    std::vector<std::string> source;
    GLuint PROGRAM_TYPE = GL_COMPUTE_SHADER;

    GLSLShaderDef(const std::string &name, const std::vector<std::string> &source, GLuint programType = GL_COMPUTE_SHADER);

    static std::string Include(const std::string& file_name) {
        return "#include \"" + file_name + "\"\n";
    }
    gl_ptr Compile() const;
private:
    mutable gl_ptr compiled_shader;
};

struct ShaderDB {
private:
    std::map<std::string, std::vector<uint8_t>> binaries;
    std::map<std::string, std::string> sources;

    std::string Checksum(const std::vector<GLSLShaderDef>& shaders);
public:
    static ShaderDB& g();

    void Register(const std::string& name, const std::string& src);
    void Register(const std::string& name, const std::vector<uint8_t>& src);
    const std::string& getSource(const std::string& name) const;

    gl_ptr Compile(const std::vector<GLSLShaderDef>& shaders);
};

void ShaderDB_Register(const std::string& name, const std::string& src);
void ShaderDB_Register(const std::string& name, const std::vector<uint8_t>& src);