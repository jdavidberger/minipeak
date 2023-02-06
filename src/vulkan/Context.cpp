#include <iostream>
#include <cmath>
#include "minipeak/vulkan/Context.h"
#include "minipeak/vulkan/VBuffer.h"

//#include <glslang/Public/ResourceLimits.h>
//#include <glslang/SPIRV/GlslangToSpv.h>
//#include "glslang/Public/ShaderLang.h"

static uint32_t getFamilyIndex(vk::raii::PhysicalDevice &PhysicalDevice) {
    std::vector<vk::QueueFamilyProperties> QueueFamilyProps = PhysicalDevice.getQueueFamilyProperties();
    auto PropIt = std::find_if(QueueFamilyProps.begin(), QueueFamilyProps.end(),
                               [](const vk::QueueFamilyProperties &Prop) {
                                   return Prop.queueFlags & vk::QueueFlagBits::eCompute;
                               });
    return std::distance(QueueFamilyProps.begin(), PropIt);
}

static const std::vector<const char *> Layers = {"VK_LAYER_KHRONOS_validation"};
static const std::vector<const char *> extensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

static float priorities[] = {1.};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkBool32 vkDebugReportCallbackEXT(
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objectType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     messageCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMessage,
        void*                                       pUserData) {
    return 0;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static vk::raii::Instance createInstance(vk::raii::Context& ctx, vk::ApplicationInfo* AppInfo) {
    try {
         return vk::raii::Instance(ctx, vk::InstanceCreateInfo(vk::InstanceCreateFlags(), // Flags
                                       AppInfo,                  // Application Info
                                       1,
                                       Layers.data(),
                                       1,
                                       extensions.data()));
    } catch(const std::exception& e) {
        fprintf(stderr, "Could not create instance with given layers %s\n", e.what());
        return vk::raii::Instance(ctx, vk::InstanceCreateInfo(vk::InstanceCreateFlags(), // Flags
                                      AppInfo,                  // Application Info
                                      0,
                                      Layers.data(),
                                      0,
                                      extensions.data()));
    }
}

Context::Context() : AppInfo(std::make_unique<vk::ApplicationInfo>(
        "VulkanCompute",      // Application Name
        1,                    // Application Version
        nullptr,              // Engine Name or nullptr
        0,                    // Engine Version
        VK_API_VERSION_1_1    // Vulkan API version
)),
                     Instance(createInstance(ctx, AppInfo.get())),
                     PhysicalDevice(Instance.enumeratePhysicalDevices().front()),
                     ComputeQueueFamilyIndex(getFamilyIndex(PhysicalDevice)),
                     DeviceQueueCreateInfo(std::make_unique<vk::DeviceQueueCreateInfo>(vk::DeviceQueueCreateFlags(),
                                                                                       ComputeQueueFamilyIndex, 1,
                                                                                       priorities)),
                     Device(PhysicalDevice,
                            vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, DeviceQueueCreateInfo.get())),
                     CommandPool(Device.createCommandPool(
                             vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), ComputeQueueFamilyIndex))),
                     PipelineCache(Device.createPipelineCache(vk::PipelineCacheCreateInfo())),
                     Queue(Device.getQueue(ComputeQueueFamilyIndex, 0)) {
    vk::PhysicalDeviceProperties DeviceProps = PhysicalDevice.getProperties();
    std::cout << "Device Name    : " << DeviceProps.deviceName << std::endl;
    const uint32_t ApiVersion = DeviceProps.apiVersion;
    std::cout << "Vulkan Version : " << VK_VERSION_MAJOR(ApiVersion) << "." << VK_VERSION_MINOR(ApiVersion) << "." << VK_VERSION_PATCH(ApiVersion) << std::endl;
    std::cout << "Driver version : " << VK_VERSION_MAJOR(DeviceProps.driverVersion) << "." << VK_VERSION_MINOR(DeviceProps.driverVersion) << "." << VK_VERSION_PATCH(DeviceProps.driverVersion) << std::endl;
    vk::PhysicalDeviceLimits DeviceLimits = DeviceProps.limits;
    std::cout << "Max Compute Shared Memory Size: " << DeviceLimits.maxComputeSharedMemorySize / 1024 << " KB" << std::endl;

    vk::PhysicalDeviceMemoryProperties MemoryProperties = PhysicalDevice.getMemoryProperties();
    for (uint32_t CurrentMemoryTypeIndex = 0;
         CurrentMemoryTypeIndex < MemoryProperties.memoryTypeCount; ++CurrentMemoryTypeIndex) {
        vk::MemoryType MemoryType = MemoryProperties.memoryTypes[CurrentMemoryTypeIndex];
        auto& heap = MemoryProperties.memoryHeaps[MemoryType.heapIndex];

        std::cout << "Mem " << CurrentMemoryTypeIndex << " Heap idx: " << MemoryType.heapIndex << " " <<
                  (heap.size/1024./1024./1024.) << "Gb " << to_string(heap.flags) << " " <<
                  to_string(MemoryType.propertyFlags) << std::endl;
    }

    VkDebugReportCallbackCreateInfoEXT ext = {
            .pfnCallback = vkDebugReportCallbackEXT
    };

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    if (CreateDebugUtilsMessengerEXT(*Instance, &createInfo, nullptr, &debugUtilsMessanger) != VK_SUCCESS) {
        //throw std::runtime_error("failed to set up debug messenger!");
    }

    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
    CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(*Instance, "vkCreateDebugReportCallbackEXT");

    if(CreateDebugReportCallback) {
        debugReportCallback = std::make_shared<VkDebugReportCallbackEXT>();
        CreateDebugReportCallback(*Instance, &ext, 0 , debugReportCallback.get());
    }
}

std::weak_ptr<Context> Context::g;

std::shared_ptr<Context> Context::Inst() {
    if (auto ptr = g.lock()) {
        return ptr;
    }
    auto rtn = std::shared_ptr<Context>(new Context());
    g = rtn;
    return rtn;
}

vk::raii::Buffer Context::CreateBuffer(size_t size) {
    vk::BufferCreateInfo BufferCreateInfo{
            vk::BufferCreateFlags(),                    // Flags
            size,                                 // Size
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,    // Usage
            vk::SharingMode::eExclusive,                // Sharing mode
            1,                                          // Number of queue family indices
            &ComputeQueueFamilyIndex                    // List of queue family indices
    };
    return Device.createBuffer(BufferCreateInfo);
}

vk::raii::Buffer Context::CreateUniform(size_t size) {
    vk::BufferCreateInfo BufferCreateInfo{
            vk::BufferCreateFlags(),                    // Flags
            size,                                 // Size
            vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,    // Usage
            vk::SharingMode::eExclusive,                // Sharing mode
            1,                                          // Number of queue family indices
            &ComputeQueueFamilyIndex                    // List of queue family indices
    };
    return Device.createBuffer(BufferCreateInfo);
}


std::tuple<vk::raii::Buffer, vk::raii::Buffer> Context::CreateTransfer(size_t size) {
    vk::BufferCreateInfo BufferCreateInfo_dst{
            vk::BufferCreateFlags(),                    // Flags
            size,                                 // Size
            vk::BufferUsageFlagBits::eTransferDst,    // Usage
            vk::SharingMode::eExclusive,                // Sharing mode
            1,                                          // Number of queue family indices
            &ComputeQueueFamilyIndex                    // List of queue family indices
    };
    vk::BufferCreateInfo BufferCreateInfo_src{
            vk::BufferCreateFlags(),                    // Flags
            size,                                 // Size
            vk::BufferUsageFlagBits::eTransferSrc,    // Usage
            vk::SharingMode::eExclusive,                // Sharing mode
            1,                                          // Number of queue family indices
            &ComputeQueueFamilyIndex                    // List of queue family indices
    };
    return std::make_tuple(Device.createBuffer(BufferCreateInfo_src), Device.createBuffer(BufferCreateInfo_dst));
}


vk::raii::DeviceMemory Context::AllocBuffer(vk::raii::Buffer &buffer, vk::Flags<vk::MemoryPropertyFlagBits> flags, vk::Flags<vk::MemoryPropertyFlagBits>* found_flags) {
    auto memoryRequirements = buffer.getMemoryRequirements();
    uint32_t MemoryTypeIndex = uint32_t(~0);

//    auto host_flags = vk::MemoryPropertyFlagBits::eHostCached | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
//    auto nonhost_flags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent;
//    auto flags = host_accessible ? host_flags : nonhost_flags;

    vk::PhysicalDeviceMemoryProperties MemoryProperties = PhysicalDevice.getMemoryProperties();
    for (uint32_t CurrentMemoryTypeIndex = 0;
         CurrentMemoryTypeIndex < MemoryProperties.memoryTypeCount; ++CurrentMemoryTypeIndex) {
        vk::MemoryType MemoryType = MemoryProperties.memoryTypes[CurrentMemoryTypeIndex];
        if ((MemoryType.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) == vk::MemoryPropertyFlagBits::eHostVisible) {
            MemoryTypeIndex = CurrentMemoryTypeIndex;
            if(found_flags) *found_flags = MemoryType.propertyFlags;
        }
        if ((MemoryType.propertyFlags & flags) == flags) {
            MemoryTypeIndex = CurrentMemoryTypeIndex;
            if(found_flags) *found_flags = MemoryType.propertyFlags;
            break;
        }
    }
    assert(MemoryTypeIndex != uint32_t(~0));
    vk::MemoryAllocateInfo AllocateInfo(memoryRequirements.size, MemoryTypeIndex);
    return Device.allocateMemory(AllocateInfo);
}

Context::~Context() {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*Instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func) {
        func(*Instance, debugUtilsMessanger, 0);
    }
}

std::vector<vk::DescriptorSetLayoutBinding>
createDescriptorSetLayoutBinding(const std::vector<std::shared_ptr<VBuffer>> &buffers) {
    std::vector<vk::DescriptorSetLayoutBinding> layout;
    for (size_t i = 0; i < buffers.size(); i++) {
        assert(buffers[i]);
        layout.emplace_back(i, buffers[i]->is_uniform() ? vk::DescriptorType::eUniformBuffer : vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
    }
    return layout;
}
//
//static inline std::vector<unsigned int> compile_from_srcs(const std::vector<std::string>& srcs) {
//    glslang::TShader shader(EShLangCompute);
//    std::vector<const char*> c_srcs;
//    for(auto& s : srcs) {
//        c_srcs.push_back(s.c_str());
//    }
//
//    shader.setStrings(c_srcs.data(), c_srcs.size());
//    shader.setEnvInput(glslang::EShSourceGlsl, EShLangCompute, glslang::EShClientVulkan, 100);
//    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
//    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_2);
//    if(!shader.parse(GetDefaultResources(), 120, false, EShMessages::EShMsgDebugInfo)) {
//
//    }
//
//    glslang::TProgram program;
//    program.addShader(&shader);
//    auto intermediate = program.getIntermediate(EShLangCompute);
//    std::vector<unsigned int> spirv;
//    if(intermediate) {
//        glslang::GlslangToSpv(*intermediate, spirv);
//    }
//    return spirv;
//}
//
//ComputeNode::ComputeNode(const char* name, const std::vector<unsigned int>& shader_bytes,
//            uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
//            const std::vector<std::shared_ptr<VBuffer>>& buffers,
//            const std::vector<vk::PushConstantRange>& pushConstantRanges,
//            vk::SpecializationInfo* SpecializationInfo) : ComputeNode(name, (uint8_t*)shader_bytes.data(), shader_bytes.size() * 4,
//                                                                          groupCountX, groupCountY, groupCountZ,
//                                                                          buffers, pushConstantRanges, SpecializationInfo) {
//
//}
//
//ComputeNode::ComputeNode(const char* name, const std::vector<std::string>& srcs,
//            uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
//            const std::vector<std::shared_ptr<VBuffer>>& buffers,
//            const std::vector<vk::PushConstantRange>& pushConstantRanges,
//            vk::SpecializationInfo* SpecializationInfo) : ComputeNode(name, compile_from_srcs(srcs),
//                                                                          groupCountX, groupCountY, groupCountZ,
//                                                                          buffers, pushConstantRanges, SpecializationInfo) {
//
//}

ComputeNode::ComputeNode(const char* name, const uint8_t *shader_bytes, size_t shader_len,
                         uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
                         const std::vector<std::shared_ptr<VBuffer>> &buffers,
                         const std::vector<vk::PushConstantRange>& pushConstantRanges,
                         vk::SpecializationInfo* SpecializationInfo) :
        ctx(Context::Inst()),
        PushConstantRanges(pushConstantRanges),
        ShaderModule(ctx->Device.createShaderModule(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
                                                                               shader_len,
                                                                               reinterpret_cast<const uint32_t *>(shader_bytes)))),
        DescriptorSetLayoutBinding(createDescriptorSetLayoutBinding(buffers)),
        DescriptorSetLayout(ctx->Device.createDescriptorSetLayout(
                vk::DescriptorSetLayoutCreateInfo(
                        vk::DescriptorSetLayoutCreateFlags(),
                        DescriptorSetLayoutBinding.size(),
                        DescriptorSetLayoutBinding.data()))),
        DescriptorPoolSizes({vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, std::max((size_t)1, buffers.size())),
                             vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, std::max((size_t)1, buffers.size()))}),
        DescriptorPool(ctx->Device.createDescriptorPool(
                vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet), 1, DescriptorPoolSizes.size(),
                                             DescriptorPoolSizes.data()))),
        Fence(ctx->Device.createFence(vk::FenceCreateInfo())),
        GroupCount({groupCountX, groupCountY, groupCountZ}),
        name(name),
        PipelineLayout(ctx->Device.createPipelineLayout(
                vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &*DescriptorSetLayout,
                                             PushConstantRanges.size(),
                                             PushConstantRanges.data()))),
        ComputePipeline(ctx->Device.createComputePipeline(ctx->PipelineCache, vk::ComputePipelineCreateInfo(
                vk::PipelineCreateFlags(vk::PipelineCreateFlagBits::eLinkTimeOptimizationEXT),    // Flags
                vk::PipelineShaderStageCreateInfo(
                        vk::PipelineShaderStageCreateFlags(),  // Flags
                        vk::ShaderStageFlagBits::eCompute,     // Stage
                        *ShaderModule,                          // Shader Module
                        "main", SpecializationInfo),     // Shader Create Info struct
                *PipelineLayout))),
        QueryPool(ctx->Device, vk::QueryPoolCreateInfo({}, vk::QueryType::eTimestamp,16)) {
    auto &Device = ctx->Device;

    size_t total_alloc = 0;
    for(auto& p : PushConstantRanges) {
        total_alloc += p.size;
    }
    if(total_alloc) {
        PushConstantRangeDataBlock = std::shared_ptr<void>(malloc(total_alloc), free);
        for(size_t i = 0;i< total_alloc / 4;i++) {
            ((float*)PushConstantRangeDataBlock.get())[i] = NAN;
        }
    }

    vk::DescriptorSetAllocateInfo DescriptorSetAllocInfo(*DescriptorPool, 1, &*DescriptorSetLayout);
    DescriptorSets = Device.allocateDescriptorSets(DescriptorSetAllocInfo);
    auto &DescriptorSet = DescriptorSets.front();

    std::vector<vk::WriteDescriptorSet> WriteDescriptorSets;

    for (size_t i = 0; i < buffers.size(); i++) {
        WriteDescriptorSets.emplace_back(*DescriptorSet, i, 0, 1, buffers[i]->is_uniform() ? vk::DescriptorType::eUniformBuffer : vk::DescriptorType::eStorageBuffer, nullptr,
                                         &buffers[i]->DescriptorBufferInfo);
    }

    Device.updateDescriptorSets(WriteDescriptorSets, {});

    vk::CommandBufferAllocateInfo CommandBufferAllocInfo(
            *ctx->CommandPool,                         // Command Pool
            vk::CommandBufferLevel::ePrimary,    // Level
            1);                                  // Num Command Buffers

    CmdBuffers = Device.allocateCommandBuffers(CommandBufferAllocInfo);
    auto &CmdBuffer = CmdBuffers.front();

    assert(groupCountX > 0);
    assert(groupCountY > 0);
    assert(groupCountZ > 0);

    vk::CommandBufferBeginInfo CmdBufferBeginInfo((vk::CommandBufferUsageFlagBits) 0);
    CmdBuffer.begin(CmdBufferBeginInfo);

    AddToCmdBuffer(CmdBuffer);

    CmdBuffer.end();
}

void ComputeNode::operator()() {
    vk::SubmitInfo SubmitInfo(0,                // Num Wait Semaphores
                              nullptr,        // Wait Semaphores
                              nullptr,        // Pipeline Stage Flags
                              1,              // Num Command Buffers
                              &*CmdBuffers.front());    // List of command buffers
    ctx->Queue.submit({SubmitInfo}, *Fence);

    auto timeout_time_ns = 500000;
    while (ctx->Device.waitForFences({*Fence},             // List of fences
                                     true,               // Wait All
                                     timeout_time_ns) == vk::Result::eTimeout) {

    }
    ctx->Device.resetFences({*Fence});
    auto results = QueryPool.getResults<uint64_t>(0, 2, 8 * 2, 8, vk::QueryResultFlagBits::e64);
    auto timestamps = results.second;
    counts += 1;
    execution_time_s += (timestamps[1] - timestamps[0]) * ctx->PhysicalDevice.getProperties().limits.timestampPeriod / 1e9;
}

ComputeNode::~ComputeNode() {
    //float avg_time_s = execution_time_s / counts;
    //printf("Avg time in %-48s %12.3fhz %12.3fms\n", name.c_str(), 1. / avg_time_s, avg_time_s * 1000.);
}

void ComputeNode::AddToCmdBuffer(vk::raii::CommandBuffer &CmdBuffer) {
    CmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *ComputePipeline);
    CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,    // Bind point
                                 *PipelineLayout,                  // Pipeline Layout
                                 0,                               // First descriptor set
                                 {*DescriptorSets.front()},               // List of descriptor sets
                                 {});                             // Dynamic offsets

    CmdBuffer.resetQueryPool(*QueryPool, 0, 2);

    CmdBuffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, *QueryPool, 0);
    size_t offset = 0;
    for(auto pushConstants : PushConstantRanges) {
        vkCmdPushConstants(*CmdBuffer, *PipelineLayout, (VkShaderStageFlags)vk::ShaderStageFlagBits::eCompute, 0, pushConstants.size, (uint8_t*)PushConstantRangeDataBlock.get() + offset);
        offset += pushConstants.size;
    }
    CmdBuffer.dispatch(GroupCount[0], GroupCount[1], GroupCount[2]);
    CmdBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *QueryPool, 1);
}

vk::Flags<vk::MemoryPropertyFlagBits> Context::alloc_flag(const BufferInfo_t& bufferInfo) const {
    auto usage = bufferInfo.usage;
    using usage_t = BufferInfo_t::usage_t;
    if((int)usage & (int)usage_t::host_available) {
        return vk::MemoryPropertyFlagBits::eHostCached | vk::MemoryPropertyFlagBits::eHostVisible;
    }
    if((int)usage & (int)usage_t::output) {
        return vk::MemoryPropertyFlagBits::eHostCached | vk::MemoryPropertyFlagBits::eHostVisible;
    }

    return vk::MemoryPropertyFlagBits::eDeviceLocal;

    if(usage == usage_t::intermediate) {;
        return vk::MemoryPropertyFlagBits::eDeviceLocal;
    }
//    if(usage == usage_t::input) {
//        return vk::MemoryPropertyFlagBits::eDeviceLocal;
//    }

    if((int)usage & (int)usage_t::output) {
        return vk::MemoryPropertyFlagBits::eHostCached | vk::MemoryPropertyFlagBits::eHostVisible;
    }

    if((int)usage & (int)usage_t::uniform) {
        return vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;
    }

    return vk::MemoryPropertyFlagBits::eHostCached | vk::MemoryPropertyFlagBits::eHostVisible;
}
