#include "minipeak/gpu/BufferInfo.h"
#include <type_traits>
#include <assert.h>

BufferInfo_t::usage_t operator | (const BufferInfo_t::usage_t lhs, const BufferInfo_t::usage_t rhs)
{
    using T = std::underlying_type_t <BufferInfo_t::usage_t>;
    return static_cast<BufferInfo_t::usage_t>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

BufferInfo_t::usage_t operator & (const BufferInfo_t::usage_t lhs, const BufferInfo_t::usage_t rhs)
{
    using T = std::underlying_type_t <BufferInfo_t::usage_t>;
    return static_cast<BufferInfo_t::usage_t>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

size_t BufferInfo_t::type_size() const {
    switch(type) {
    case 'h':
        case 's':
            return 2;
        case 'b':
            return 1;
        case 'f':
            return sizeof(float);
        case 'd':
            return sizeof(double);
        default:	  
            assert(false);
            return 1;
    }
}

size_t BufferInfo_t::buffer_size() const {
    return w * h * c * type_size();
}

BufferInfo_t::BufferInfo_t(int type, int c, int w, int h, enum usage_t usage, BufferInfo_t::order_t order)
        : w(w), h(h), c(c), type(type), usage(usage), order(order) {}

std::string BufferInfo_t::cl_type(int vec_size) const {
    if(vec_size == 1) {
        switch (type) {
            case 's':
                return "short";
            case 'b':
                return "unsigned char";
	case 'h':
	  return "half";
            case 'f':
                return "float";
            case 'd':
                return "double";
            default:
                assert(false);
                return "void";
                break;
        }
    } else {
        auto base = cl_type(1);
        return base + std::to_string(vec_size);
    }
}

BufferInfo_t::BufferInfo_t(const std::string &n, int type, int c, int w, int h, BufferInfo_t::usage_t usage,
                           BufferInfo_t::order_t order) : name(n), w(w), h(h), c(c), type(type), usage(usage), order(order) {

}
