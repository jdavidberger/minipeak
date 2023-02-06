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

GLSLBuffer::GLSLBuffer(const BufferInfo_t &info) : GPUBuffer(info) {
    this->ptr = generate_buffer();
    //printf("Generated %u for %s\n", *this->ptr, info.name.c_str());
    glBindBuffer(target(), *ptr);

    bool persistent = (info.usage & BufferInfo_t::usage_t::host_available) == BufferInfo_t::usage_t::host_available ||
            (info.usage & BufferInfo_t::usage_t::input) == BufferInfo_t::usage_t::input ||
            (info.usage & BufferInfo_t::usage_t::output) == BufferInfo_t::usage_t::output;
    if(persistent) {
        int flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        //printf("Buffer data with flags 0x%x for %s\n", flags, info.name.c_str());
        glBufferStorage(target(), buffer_size(), 0, flags);
        this->mapped_ptr = glMapBufferRange(target(), 0, buffer_size(), flags);
        assert(this->mapped_ptr);
    } else {
        bool useBufferStorage = true;
        if(useBufferStorage) {
            int flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
            glBufferStorage(target(), buffer_size(), 0, flags);
        } else {
            auto type = info.is_static() ? GL_STATIC_DRAW : GL_DYNAMIC_READ;
            if (info.usage == BufferInfo_t::usage_t::intermediate) {
                type = GL_DYNAMIC_COPY;
            }

            printf("Buffer data with type 0x%x for %s\n", type, info.name.c_str());
            glBufferData(target(), (GLsizeiptrARB) buffer_size(), nullptr, type);
        }
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
        //void *src = glMapBuffer(target(), GL_READ_ONLY);
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
            if(vec > 1) {
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
        default: assert(false); return "void"; break;
    }
}

std::string GLSLBuffer::glsl_constants(const std::string& prefix, int vec_size) const {
    auto order = info.order;
    auto type = info.type;

    if ((info.w % vec_size) != 0) {
        throw std::runtime_error("Invalid vectorization size");
    }

    std::stringstream ss;
    ss << "#define " << prefix << "dtype " << glsl_type(vec_size) << std::endl;
    ss << "#define " << prefix << "order " << (int)order << std::endl;
    if(order == order_t::CHW) {
        if(type=='s' && !has_int16) {
            ss << "#define ACCESS16(d,idx) ((d[(idx)/2u] >> (16u*(((idx))%2u))) & 0xffff)" << std::endl;
            ss << "#define " << prefix << "FLAT_ACCESS_IDX(c,x,y) (uint(uint(c) * (" << prefix << "width * " << prefix
               << "height) + uint(x) * (" << prefix << "width) + uint(y)))" << std::endl;

            ss << "#define " << prefix << "FLAT_ACCESS(d,c,x,y) ACCESS16(d, "<< prefix << "FLAT_ACCESS_IDX(c,x,y))" << std::endl;
        } else {
            ss << "#define " << prefix << "ACCESS(d,c,x,y) d[c][x][y]" << std::endl;
            ss << "#define " << prefix << "FLAT_ACCESS_1D(d,c,xy) d[uint(uint(c) * (" << prefix << "width * " << prefix
               << "height) + uint(xy))]" << std::endl;
            ss << "#define " << prefix << "FLAT_ACCESS(d,c,x,y) d[uint(uint(c) * (" << prefix << "width * " << prefix
               << "height) + uint(x) * (" << prefix << "width) + uint(y))]" << std::endl;
        }
    } else {
        ss << "#define " << prefix << "ACCESS(d,c,x,y) d[x][y][c]" << std::endl;
        ss << "#define " << prefix << "FLAT_ACCESS(d,c,x,y) d[uint(uint(x) * ("<< prefix <<"width * "<< prefix <<"channels) + uint(y) * ("<< prefix <<"channels) + uint(c))]" << std::endl;
    }
    auto c = info.c;
    auto w = info.w;
    auto h = info.h;
    ss << "#define convert_" << prefix << " " << glsl_type(vec_size) << std::endl;
    ss << "#define " << prefix << "channels " << c << "u" << std::endl;
    ss << "#define " << prefix << "width " << (w/vec_size) << "u" << std::endl;
    ss << "#define " << prefix << "height " << h << "u" << std::endl;
    return ss.str();
}
