//
// Created by justin on 12/27/22.
//

#include "minipeak/vulkan/VBuffer.h"
#include "minipeak/vulkan/Context.h"

VBuffer::VBuffer(char type, int channels, int width, int height, enum BufferInfo_t::usage_t usage, BufferInfo_t::order_t order) :
        VBuffer(BufferInfo_t(type, channels, width, height, usage, order)){

}

size_t VBuffer::buffer_size() const {
    return info.buffer_size();
}

size_t VBuffer::type_size() const {
    return info.type_size();
}

void VBuffer::read(void *dst) const {
    if(dst == 0) return;
    if(mapped_ptr == 0) {
        vk::SubmitInfo SubmitInfo(0,                // Num Wait Semaphores
                                  nullptr,        // Wait Semaphores
                                  nullptr,        // Pipeline Stage Flags
                                  1,              // Num Command Buffers
                                  &*CmdBuffers[1]);    // List of command buffers
        ctx->Queue.submit({SubmitInfo}, *Fence);

        auto timeout_time_ns = 500000;
        while (ctx->Device.waitForFences({*Fence},             // List of fences
                                         true,               // Wait All
                                         timeout_time_ns) == vk::Result::eTimeout) {
        }
        ctx->Device.resetFences({*Fence});

        this->stage_dst->read(dst);
        return;
    }

    if((this->MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlagBits::eHostCoherent) {
        ctx->Device.flushMappedMemoryRanges({
            vk::MappedMemoryRange(
                    *DeviceMemory, 0, Buffer.getMemoryRequirements().size
                    )
        });
    }

    memcpy(dst, mapped_ptr, buffer_size());
}

void VBuffer::write(const void *src) {
    if(src == 0) return;
    if(mapped_ptr == 0) {
        this->stage_src->write(src);

        vk::SubmitInfo SubmitInfo(0,                // Num Wait Semaphores
                                  nullptr,        // Wait Semaphores
                                  nullptr,        // Pipeline Stage Flags
                                  1,              // Num Command Buffers
                                  &*CmdBuffers[0]);    // List of command buffers
        ctx->Queue.submit({SubmitInfo}, *Fence);

        auto timeout_time_ns = 500000;
        while (ctx->Device.waitForFences({*Fence},             // List of fences
                                         true,               // Wait All
                                         timeout_time_ns) == vk::Result::eTimeout) {

        }
        ctx->Device.resetFences({*Fence});

        return;
    }

    memcpy(mapped_ptr, src, buffer_size());
	
    if((this->MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlagBits::eHostCoherent) {
        ctx->Device.invalidateMappedMemoryRanges({
                                                    vk::MappedMemoryRange(
                                                            *DeviceMemory, 0, Buffer.getMemoryRequirements().size
                                                    )
                                            });
    }
}

VBuffer::~VBuffer() {
    if(mapped_ptr) {
        DeviceMemory.unmapMemory();
    }
}

VBuffer::VBuffer(const BufferInfo_t &info, vk::raii::Buffer&& _Buffer) : ctx(Context::Inst()), info(info),
                                                                       Buffer(std::forward<vk::raii::Buffer>(_Buffer)),
                                                                       DescriptorBufferInfo(vk::DescriptorBufferInfo(*Buffer, 0, buffer_size())),
                                                                       DeviceMemory(Context::Inst()->AllocBuffer(Buffer, ctx->alloc_flag(info), &MemoryPropertyFlags)),
                                                                       Fence(ctx->Device.createFence(vk::FenceCreateInfo())) {
    Buffer.bindMemory(*DeviceMemory, 0);
    auto result = VK_ERROR_MEMORY_MAP_FAILED;
    if(MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) {
        result = vkMapMemory(*ctx->Device, *DeviceMemory, 0,  Buffer.getMemoryRequirements().size, 0, &mapped_ptr);
    }
    if(result != VK_SUCCESS) {
        mapped_ptr = 0;
        vk::BufferCreateInfo BufferCreateInfo_dst{
                vk::BufferCreateFlags(),                    // Flags
                buffer_size(),                                 // Size
                vk::BufferUsageFlagBits::eTransferDst,    // Usage
                vk::SharingMode::eExclusive,                // Sharing mode
        };
        vk::BufferCreateInfo BufferCreateInfo_src{
                vk::BufferCreateFlags(),                    // Flags
                buffer_size(),                                 // Size
                vk::BufferUsageFlagBits::eTransferSrc,    // Usage
                vk::SharingMode::eExclusive,                // Sharing mode
        };
        auto new_info = info;
        new_info.usage = BufferInfo_t::usage_t::host_available;
        stage_src = std::make_shared<VBuffer>(new_info, ctx->Device.createBuffer(BufferCreateInfo_src));
        new_info.usage = BufferInfo_t::usage_t::host_available;
        stage_dst = std::make_shared<VBuffer>(new_info, ctx->Device.createBuffer(BufferCreateInfo_dst));

        vk::CommandBufferAllocateInfo CommandBufferAllocInfo(
                *ctx->CommandPool,                         // Command Pool
                vk::CommandBufferLevel::ePrimary,    // Level
                2);                                  // Num Command Buffers

        CmdBuffers = ctx->Device.allocateCommandBuffers(CommandBufferAllocInfo);

        vk::CommandBufferBeginInfo CmdBufferBeginInfo((vk::CommandBufferUsageFlagBits) 0);
        CmdBuffers[0].begin(CmdBufferBeginInfo);
        CmdBuffers[0].copyBuffer(*stage_src->Buffer, *Buffer, {vk::BufferCopy(0, 0, buffer_size())});
        CmdBuffers[0].end();

        CmdBuffers[1].begin(CmdBufferBeginInfo);
        CmdBuffers[1].copyBuffer(*Buffer, *stage_dst->Buffer, {vk::BufferCopy(0, 0, buffer_size())});
        CmdBuffers[1].end();
    }
}

VBuffer::VBuffer(const BufferInfo_t &info) : VBuffer(info, info.uniform() ? Context::Inst()->CreateUniform(info.buffer_size()) : Context::Inst()->CreateBuffer(info.buffer_size())) {

}

std::shared_ptr<VBuffer> VBuffer::copy() const {
    return std::make_shared<VBuffer>(info);
}

bool VBuffer::is_host_visible() const {
    return mapped_ptr != 0;
}

void VBuffer::stage_write(const void *src) {
    if(this->stage_src) {
        this->stage_src->write(src);
    } else {
        write(src);
    }
}

void VBuffer::stage_read(void *dst) const {
    if(this->stage_dst) {
        this->stage_dst->read(dst);
    } else {
        read(dst);
    }

}

bool VBuffer::is_uniform() const {
    return info.uniform();
}
