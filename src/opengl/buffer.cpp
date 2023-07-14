#include <cassert>
#include <cstring>
#include "minipeak/opengl/buffer.h"
#include "sstream"

static void delete_buffer(const GLuint *s)
{
    if (s && *s) {
        glDeleteBuffers(1, s);
    }
}


static gl_ptr generate_buffer() {
    GLuint val;
    glGenBuffers(1, &val);
    return gl_ptr(new GLuint (val), delete_buffer);
}

GLSLBuffer::GLSLBuffer(const BufferInfo_t& info, bool persistent, bool bufferStorage, int flags) : GPUBuffer(info) {
    this->ptr = generate_buffer();
    //printf("Generated %u for %s\n", *this->ptr, info.name.c_str());
    glBindBuffer(target(), *ptr);
    if(bufferStorage) {
        glBufferStorage(target(), buffer_size(), 0, flags);

        if(persistent) {
          this->mapped_ptr = glMapBufferRange(target(), 0, buffer_size(), flags);
          assert(this->mapped_ptr);
        }

    } else {
      glBufferData(target(), (GLsizeiptrARB) buffer_size(), nullptr, flags);
    }
     
    glBindBuffer(target(), 0);
    assert(this->ptr);
}

GLSLBuffer::GLSLBuffer(const BufferInfo_t &info) : GPUBuffer(info) {
    this->ptr = generate_buffer();
    //printf("Generated %u for %s\n", *this->ptr, info.name.c_str());
    glBindBuffer(target(), *ptr);

    bool persistent =
      (info.usage & BufferInfo_t::usage_t::host_available) == BufferInfo_t::usage_t::host_available ||
            (info.usage & BufferInfo_t::usage_t::input) == BufferInfo_t::usage_t::input ||
            (info.usage & BufferInfo_t::usage_t::output) == BufferInfo_t::usage_t::output;
    if(persistent) {
        int flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        //printf("Buffer data with flags 0x%x for %s\n", flags, info.name.c_str());
        glBufferStorage(target(), buffer_size(), 0, flags);
        this->mapped_ptr = glMapBufferRange(target(), 0, buffer_size(), flags);
        assert(this->mapped_ptr);
    } else {
        bool useBufferStorage = false;
        if(useBufferStorage) {
            int flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
            glBufferStorage(target(), buffer_size(), 0, flags);
        } else {
            auto type = info.is_static() ? GL_STATIC_DRAW : GL_DYNAMIC_READ;
            if (info.usage == BufferInfo_t::usage_t::intermediate) {
                type = GL_DYNAMIC_COPY;
            }
            glBufferData(target(), (GLsizeiptrARB) buffer_size(), nullptr, type);
        }
    }

    {
        auto ptr = calloc(1, buffer_size());
        write(ptr);
        free(ptr);
    }
    glBindBuffer(target(), 0);
    assert(this->ptr);
}

void GLSLBuffer::read(void *dst) const{
    if(dst == 0) return;

    if(this->mapped_ptr) {
        memcpy(dst, this->mapped_ptr, buffer_size());
    } else {
      glBindBuffer(target(), *ptr);
      //glGetBufferSubData(target(), 0, buffer_size(), dst);
      //void *src = glMapBuffer(target(), GL_MAP_READ_BIT);
      void *src = glMapBufferRange(target(), 0, buffer_size(), GL_MAP_READ_BIT);
      if(src) {
	memcpy(dst, src, buffer_size());
	glUnmapBuffer(target());
      }

        glBindBuffer(target(), 0);
    }
}

void GLSLBuffer::write(const void* src) {
    if(src == 0) return;

    if(this->mapped_ptr) {
        memcpy(this->mapped_ptr, src, buffer_size());
    } else {
        glBindBuffer(target(), *ptr);

//        glBufferSubData(target(), 0, buffer_size(), src);
        void *dst = glMapBufferRange(target(), 0, buffer_size(),  GL_MAP_WRITE_BIT);
        assert(dst);
        if(dst) {
            memcpy(dst, src, buffer_size());
            glUnmapBuffer(target());
        }
        glBindBuffer(target(), 0);
    }
}

bool has_int16 = false;

std::string GLSLBuffer::glsl_type(int vec) const {
    std::string input_type;
    //assert(vec == 1 || info.type == 'f');
    switch(info.type) {
        case 's':
            if(vec > 2) {
                return "ivec" + std::to_string(vec/2);
            }
            return has_int16 ? "int16_t" : "int";
        case 'b': return "unsigned char";
        case 'f':
            if(vec > 1) {
                return "vec" + std::to_string(vec);
            }
            return "float";
        case 'd': return "double";
        default: assert(false);
	  return "void"; break;
    }
}

std::string GLSLBuffer::work_type(int vec) const {
    std::string input_type;
    //assert(vec == 1 || info.type == 'f');
    switch(info.type) {
        case 's':
            if(vec > 1) {
                return "ivec" + std::to_string(vec);
            }
            return has_int16 ? "int16_t" : "int";
        case 'b': return "unsigned char";
        case 'f':
            if(vec > 1) {
                return "vec" + std::to_string(vec);
            }
            return "float";
        case 'd': return "double";
        default: assert(false);
	  return "void"; break;
    }
}

std::string GLSLBuffer::glsl_constants(const std::string& _prefix, int vec_size) const {
    auto order = info.order;
    auto type = info.type;

    auto prefix = _prefix;
    if(!prefix.empty() && prefix[prefix.size()-1] != '_')
      prefix = prefix + "_";

    if ((info.w % vec_size) != 0) {
        throw std::runtime_error("Invalid vectorization size");
    }

    auto c = info.c;
    auto w = info.w / vec_size;
    auto h = info.h;

    std::stringstream info_line;
    info_line << "d,c,x,y," << (int)info.order << "," << c << "u," << w << "u," << h <<"u";
    
    std::string vec_str = vec_size == 1 ? "" : std::to_string(vec_size);
    std::string type_str = std::string(1, info.type) + vec_str;
    std::stringstream ss;
    ss << "#define " << prefix << "work_dtype " << work_type(vec_size) << std::endl;

    if(vec_size == 1) {
        ss << "#define " << prefix << "work_dtype_access(x, n) (x)" << std::endl;
    } else {
        ss << "#define " << prefix << "work_dtype_access(x, n) ((x)[(n)])" << std::endl;
    }

    ss << "#define " << prefix << "dtype " << glsl_type(vec_size) << std::endl;
    ss << "#define " << prefix << "vec_size " << vec_size << std::endl;    
    ss << "#define " << prefix << "order " << (int)order << std::endl;
    ss << "#define " << prefix << "FLAT_ACCESS(d,c,x,y) MINIPEAK_ACCESS_" << type_str << "(" << info_line.str() << ")" << std::endl;
    ss << "#define " << prefix << "ACCESS(d,c,x,y) MINIPEAK_ACCESS_" << type_str << "(" << info_line.str() << ")" << std::endl;
    ss << "#define " << prefix << "ACCESS_1D MINIPEAK_ACCESS_1D_" << type_str << std::endl;
    
    ss << "#define " << prefix << "STORE(d,c,x,y,v) MINIPEAK_STORE_" << type_str << "(" << info_line.str() << ",v)" << std::endl;
    ss << "#define " << prefix << "STORE_1D MINIPEAK_STORE_1D_" << type_str << std::endl;            
    ss << "#define " << prefix << "CIDX_TO_1D_IDX(c,idx) MINIPEAK_CIDX_TO_1D_IDX(c,idx," << (int)info.order << "," << c << "u," << w << "u," << h <<"u)" << std::endl;
    
    ss << "#define CR_" << prefix << "DATATYPE_IS_" << glsl_type(vec_size) << " 1" << std::endl;
    ss << "#define convert_" << prefix << " " << glsl_type(vec_size) << std::endl;
    ss << "#define " << prefix << "channels " << c << "u" << std::endl;
    ss << "#define " << prefix << "width " << w << "u" << std::endl;
    ss << "#define " << prefix << "height " << h << "u" << std::endl;
    return ss.str();
}

void GLSLBuffer::swap(GLSLBuffer &buffer) {
    //assert(info == buffer.info);
    auto d = *buffer.ptr.get();
    *buffer.ptr = *ptr.get();
    *this->ptr = d;
}

