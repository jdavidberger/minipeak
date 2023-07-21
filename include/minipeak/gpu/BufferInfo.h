#pragma once
#include "stdlib.h"
#include "stdint.h"
#include "string"

//#include "vulkan/vulkan_raii.hpp"

struct BufferInfo_t {
    std::string name = "unknown";
    enum class order_t {
        CHW,
        HWC
    };

    enum class usage_t : uint32_t {
        intermediate = 0,
        input = 1,
        output = 2,
        uniform = 4,
        host_available = 8,
        data_static = 16,
        writeable = 32
    };

  //vk::Flags<vk::MemoryPropertyFlagBits> alloc_flag() const;
    bool uniform() const {
        return ((int)usage & (int)usage_t::uniform) > 0;
    }
    BufferInfo_t(const std::string& n, int type, int c, int w, int h, enum usage_t usage = usage_t::intermediate, order_t order = order_t::CHW);
    BufferInfo_t(const std::string& n, int type, int c, int w, int h, order_t order) :
            BufferInfo_t(n, type, c, w, h, usage_t::intermediate, order) {}
    BufferInfo_t(int type, int c, int w, int h, enum usage_t usage = usage_t::intermediate, order_t order = order_t::CHW);
    BufferInfo_t(int type, int c, int w, int h, order_t order) : BufferInfo_t(type, c, w, h, usage_t::intermediate, order) {}
    BufferInfo_t() = default;
    int w = 0, h = 0, c = 0, type = 0;
    enum usage_t usage = usage_t::intermediate;
    enum order_t order = order_t::CHW;


    size_t type_size() const;
    size_t element_count() const;
    size_t buffer_size() const;

    bool is_static() const {
        return ((int)usage & (int)usage_t::data_static) > 0;
    }

    std::string cl_type(int vec_size = 1) const;
};

BufferInfo_t::usage_t operator | (const BufferInfo_t::usage_t lhs, const BufferInfo_t::usage_t rhs);
BufferInfo_t::usage_t operator & (const BufferInfo_t::usage_t lhs, const BufferInfo_t::usage_t rhs);

