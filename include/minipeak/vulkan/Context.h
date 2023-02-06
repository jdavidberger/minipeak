#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "minipeak/gpu/BufferInfo.h"

class Context {
    static std::weak_ptr<Context> g;

    Context();

public:
    vk::Flags<vk::MemoryPropertyFlagBits> alloc_flag(const BufferInfo_t& bufferInfo) const;
    ~Context();
    std::unique_ptr<vk::ApplicationInfo> AppInfo;
    vk::raii::Context ctx;
    vk::raii::Instance Instance;
    vk::raii::PhysicalDevice PhysicalDevice;
    uint32_t ComputeQueueFamilyIndex = 0;
    std::unique_ptr<vk::DeviceQueueCreateInfo> DeviceQueueCreateInfo;
    vk::raii::Device Device;
    vk::raii::CommandPool CommandPool;
    vk::raii::PipelineCache PipelineCache;
    vk::raii::Queue Queue;

    std::shared_ptr<VkDebugReportCallbackEXT> debugReportCallback;

    VkDebugUtilsMessengerEXT debugUtilsMessanger;
    vk::raii::Buffer CreateBuffer(size_t size);
    vk::raii::Buffer CreateUniform(size_t size);
    std::tuple<vk::raii::Buffer, vk::raii::Buffer> CreateTransfer(size_t size);
    vk::raii::DeviceMemory AllocBuffer(vk::raii::Buffer& buffer, vk::Flags<vk::MemoryPropertyFlagBits> flags, vk::Flags<vk::MemoryPropertyFlagBits>* found_flags);

    static std::shared_ptr<Context> Inst();


};

struct VBuffer;
class ComputeNode {
protected:
    std::shared_ptr<Context> ctx;
    std::vector<vk::PushConstantRange> PushConstantRanges;
    std::shared_ptr<void> PushConstantRangeDataBlock;
    std::vector<void*> PushConstantRangeDatas;

    std::vector<vk::raii::CommandBuffer> CmdBuffers;

    vk::raii::ShaderModule ShaderModule;
    std::vector<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutBinding;
    vk::raii::DescriptorSetLayout DescriptorSetLayout;

    std::vector<vk::DescriptorPoolSize> DescriptorPoolSizes;
    vk::raii::DescriptorPool DescriptorPool;
    vk::raii::Fence Fence;
    std::array<uint32_t, 3> GroupCount;
public:
//    ComputeNode(const char* name, const std::vector<std::string>& srcs,
//                uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
//                const std::vector<std::shared_ptr<VBuffer>>& buffers,
//                const std::vector<vk::PushConstantRange>& pushConstantRanges = {},
//                vk::SpecializationInfo* SpecializationInfo = 0);
//
//    ComputeNode(const char* name, const std::vector<unsigned int>& shader_bytes,
//                uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
//                const std::vector<std::shared_ptr<VBuffer>>& buffers,
//                const std::vector<vk::PushConstantRange>& pushConstantRanges = {},
//                vk::SpecializationInfo* SpecializationInfo = 0);

    ComputeNode(const char* name, const uint8_t* shader_bytes, size_t shader_len,
                uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
                const std::vector<std::shared_ptr<VBuffer>>& buffers,
                const std::vector<vk::PushConstantRange>& pushConstantRanges = {},
                vk::SpecializationInfo* SpecializationInfo = 0);

    std::string name;
    size_t counts = 0;
    double execution_time_s = 0;

    virtual ~ComputeNode();
    virtual void operator()();
    void sync() {}

    vk::raii::PipelineLayout PipelineLayout;
    vk::raii::Pipeline ComputePipeline;
    std::vector<vk::raii::DescriptorSet> DescriptorSets;
    vk::raii::QueryPool QueryPool;

    virtual void AddToCmdBuffer(vk::raii::CommandBuffer& buffer);
};