#pragma once

#include "vulkan/vulkan_raii.hpp"

#include <string>
#include "stdlib.h"
#include "stdint.h"
#include "minipeak/gpu/BufferInfo.h"

struct Context;
class VBuffer {
public:
    std::vector<vk::raii::CommandBuffer> CmdBuffers;
    std::shared_ptr<Context> ctx;
    using order_t = BufferInfo_t::order_t;
    void *mapped_ptr = nullptr;

    BufferInfo_t info;
    vk::raii::Buffer Buffer;
    vk::DescriptorBufferInfo DescriptorBufferInfo;
    vk::Flags<vk::MemoryPropertyFlagBits> MemoryPropertyFlags;
    vk::raii::DeviceMemory DeviceMemory;
    std::shared_ptr<VBuffer> stage_src, stage_dst;
    vk::raii::Fence Fence;

    std::shared_ptr<VBuffer> copy() const;

    ~VBuffer();
    VBuffer(const VBuffer&) = default;
    VBuffer(VBuffer&&) = default;
    VBuffer(const BufferInfo_t& info, vk::raii::Buffer&& Buffer);
    VBuffer(const BufferInfo_t& info);
    VBuffer(char type, int channels, int width, int height, enum BufferInfo_t::usage_t usage = BufferInfo_t::usage_t::intermediate, enum BufferInfo_t::order_t order = BufferInfo_t::order_t::CHW);
    VBuffer& operator=(const VBuffer&) = default;

    size_t type_size() const;
    size_t buffer_size() const;

    void read(void* dst) const;
    void write(const void* src);

    void stage_read(void* dst) const;
    void stage_write(const void* src);

    bool is_host_visible() const;
    bool is_uniform() const;
};

typedef std::shared_ptr<VBuffer> VBufferPtr;