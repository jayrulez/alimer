//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// The implementation is based on WickedEngine graphics code, MIT license (https://github.com/turanszkij/WickedEngine/blob/master/LICENSE.md)

#include "AlimerConfig.h"
#include "Core/Hash.h"
#include "Core/Log.h"
#include "Graphics/Graphics.h"
#include "Graphics/Graphics_Internal.h"

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#include <volk.h>
#define VMA_IMPLEMENTATION
#include "spirv_reflect.hpp"
#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#ifdef SDL2
#    include "sdl2.h"
#    include <SDL2/SDL_vulkan.h>
#endif

#if !defined(ALIMER_DISABLE_SHADER_COMPILER) && defined(VK_USE_PLATFORM_WIN32_KHR)
#    include <dxcapi.h>
#endif

const char* VkResultToString(VkResult result)
{
    switch (result)
    {
#define STR(r)   \
    case VK_##r: \
        return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
        default:
            return "UNKNOWN_ERROR";
    }
}

/// Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                   \
    do                                                                \
    {                                                                 \
        VkResult err = x;                                             \
        if (err)                                                      \
        {                                                             \
            LOGE("Detected Vulkan error: {}", VkResultToString(err)); \
        }                                                             \
    } while (0)

#define VK_LOG_ERROR(result, message) LOGE("{} - Vulkan error: {}", message, VkResultToString(result));

// Enabling ray tracing might crash RenderDoc:
#define ENABLE_RAYTRACING_EXTENSION

// These shifts are made so that Vulkan resource bindings slots don't interfere with each other across shader stages:
//	These are also defined in compile_shaders_spirv.py as shift numbers, and it needs to be synced!
#define VULKAN_BINDING_SHIFT_B 0
#define VULKAN_BINDING_SHIFT_T 1000
#define VULKAN_BINDING_SHIFT_U 2000
#define VULKAN_BINDING_SHIFT_S 3000

namespace alimer
{
    class GraphicsDevice_Vulkan;
    class Vulkan_CommandList;

    struct FrameResources;

    struct QueueFamilyIndices
    {
        int graphicsFamily = -1;
        int presentFamily = -1;
        int copyFamily = -1;

        bool isComplete()
        {
            return graphicsFamily >= 0 && presentFamily >= 0 && copyFamily >= 0;
        }
    };

    struct DescriptorTableFrameAllocator
    {
        GraphicsDevice_Vulkan* device;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        uint32_t poolSize = 256;

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkBufferView> texelBufferViews;
        std::vector<VkWriteDescriptorSetAccelerationStructureNV> accelerationStructureViews;
        bool dirty = false;

        const GraphicsBuffer* CBV[GPU_RESOURCE_HEAP_CBV_COUNT];
        const GPUResource* SRV[GPU_RESOURCE_HEAP_SRV_COUNT];
        int SRV_index[GPU_RESOURCE_HEAP_SRV_COUNT];
        const GPUResource* UAV[GPU_RESOURCE_HEAP_UAV_COUNT];
        int UAV_index[GPU_RESOURCE_HEAP_UAV_COUNT];
        const Sampler* SAM[GPU_SAMPLER_HEAP_COUNT];

        void init(GraphicsDevice_Vulkan* device);
        void destroy();

        void reset();
        void validate(bool graphics, Vulkan_CommandList* commandList, bool raytracing = false);
        VkDescriptorSet commit(const DescriptorTable* table);
    };

    struct ResourceFrameAllocator
    {
        GraphicsDevice_Vulkan* device = nullptr;
        RefPtr<GraphicsBuffer> buffer;
        uint8_t* dataBegin = nullptr;
        uint8_t* dataCur = nullptr;
        uint8_t* dataEnd = nullptr;

        void init(GraphicsDevice_Vulkan* device, size_t size);

        uint8_t* Allocate(const uint32_t size, VkDeviceSize alignment);
        void clear();
        uint64_t calculateOffset(uint8_t* address);
    };

    class Vulkan_CommandList final : public CommandList
    {
        friend struct DescriptorTableFrameAllocator;

    public:
        Vulkan_CommandList(GraphicsDevice_Vulkan* device, uint32_t index, uint32_t queueFamilyIndex);
        ~Vulkan_CommandList() override;

        inline VkCommandBuffer GetDirectCommandList()
        {
            return commandBuffers[frameIndex];
        }

        void Reset(uint32_t frameIndex);
        VkCommandBuffer End();

        void PresentBegin() override;
        void PresentEnd() override;

        void PushDebugGroup(const char* name) override;
        void PopDebugGroup() override;
        void InsertDebugMarker(const char* name) override;

        void RenderPassBegin(const RenderPass* renderpass) override;
        void RenderPassEnd() override;
        void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) override;
        void SetViewport(const Viewport& viewport) override;
        void SetViewports(uint32_t viewportCount, const Viewport* pViewports) override;
        void SetScissorRect(const ScissorRect& rect) override;
        void SetScissorRects(uint32_t scissorCount, const ScissorRect* rects) override;
        void BindResource(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource = -1) override;
        void BindResources(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count) override;
        void BindUAV(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource = -1) override;
        void BindUAVs(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count) override;
        void BindSampler(ShaderStage stage, const Sampler* sampler, uint32_t slot) override;
        void BindConstantBuffer(ShaderStage stage, const GraphicsBuffer* buffer, uint32_t slot) override;
        void BindVertexBuffers(const GraphicsBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets) override;
        void BindIndexBuffer(const GraphicsBuffer* indexBuffer, IndexFormat format, uint32_t offset) override;
        void BindStencilRef(uint32_t value) override;
        void BindBlendFactor(float r, float g, float b, float a) override;

        void SetRenderPipeline(RenderPipeline* pipeline) override;
        void BindComputeShader(const Shader* shader) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) override;
        void DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;
        void DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;

        void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
        void DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;
        void CopyResource(const GPUResource* pDst, const GPUResource* pSrc) override;

        GPUAllocation AllocateGPU(const uint32_t size) override;
        void UpdateBuffer(GraphicsBuffer* buffer, const void* data, uint64_t size = 0) override;

        void QueryBegin(const GPUQuery* query) override;
        void QueryEnd(const GPUQuery* query) override;
        void Barrier(const GPUBarrier* barriers, uint32_t numBarriers) override;

    private:
        void FlushPipeline();
        void PrepareDraw();
        void PrepareDispatch();
        void PrepareRaytrace();

    private:
        GraphicsDevice_Vulkan* device;
        uint32_t index;

        VkCommandPool commandPools[kMaxInflightFrames] = {};
        VkCommandBuffer commandBuffers[kMaxInflightFrames] = {};
        uint32_t frameIndex = 0;

        VkViewport viewports[kMaxViewportAndScissorRects] = {};
        VkRect2D scissors[kMaxViewportAndScissorRects] = {};

        const RenderPass* active_renderpass = nullptr;
        size_t prev_pipeline_hash = 0;
        RenderPipeline* active_pso = nullptr;
        const Shader* active_cs = nullptr;
        const RaytracingPipelineState* active_rt = nullptr;
        bool dirty_pso = false;

        DescriptorTableFrameAllocator descriptors[kMaxInflightFrames];
        ResourceFrameAllocator resourceBuffer[kMaxInflightFrames];

        std::vector<std::pair<size_t, VkPipeline>> pipelines_worker;
    };

    class GraphicsDevice_Vulkan final : public Graphics
    {
        friend class Vulkan_CommandList;
        friend struct DescriptorTableFrameAllocator;

    private:
        bool debugUtils = false;
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugUtilsMessenger{VK_NULL_HANDLE};
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        QueueFamilyIndices queueIndices;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties2 device_properties = {};
        VkPhysicalDeviceVulkan11Properties device_properties_1_1 = {};
        VkPhysicalDeviceVulkan12Properties device_properties_1_2 = {};
        VkPhysicalDeviceRayTracingPropertiesKHR raytracing_properties = {};
        VkPhysicalDeviceMeshShaderPropertiesNV mesh_shader_properties = {};

        VkPhysicalDeviceFeatures2 device_features2 = {};
        VkPhysicalDeviceVulkan11Features features_1_1 = {};
        VkPhysicalDeviceVulkan12Features features_1_2 = {};
        VkPhysicalDeviceRayTracingFeaturesKHR raytracing_features = {};
        VkPhysicalDeviceMeshShaderFeaturesNV mesh_shader_features = {};

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        uint32_t swapChainImageIndex = 0;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkRenderPass defaultRenderPass = VK_NULL_HANDLE;

        VkBuffer nullBuffer = VK_NULL_HANDLE;
        VmaAllocation nullBufferAllocation = VK_NULL_HANDLE;
        VkBufferView nullBufferView = VK_NULL_HANDLE;
        VkSampler nullSampler = VK_NULL_HANDLE;
        VmaAllocation nullImageAllocation1D = VK_NULL_HANDLE;
        VmaAllocation nullImageAllocation2D = VK_NULL_HANDLE;
        VmaAllocation nullImageAllocation3D = VK_NULL_HANDLE;
        VkImage nullImage1D = VK_NULL_HANDLE;
        VkImage nullImage2D = VK_NULL_HANDLE;
        VkImage nullImage3D = VK_NULL_HANDLE;
        VkImageView nullImageView1D = VK_NULL_HANDLE;
        VkImageView nullImageView1DArray = VK_NULL_HANDLE;
        VkImageView nullImageView2D = VK_NULL_HANDLE;
        VkImageView nullImageView2DArray = VK_NULL_HANDLE;
        VkImageView nullImageViewCube = VK_NULL_HANDLE;
        VkImageView nullImageViewCubeArray = VK_NULL_HANDLE;
        VkImageView nullImageView3D = VK_NULL_HANDLE;

        uint64_t timestamp_frequency = 0;
        VkQueryPool querypool_timestamp = VK_NULL_HANDLE;
        VkQueryPool querypool_occlusion = VK_NULL_HANDLE;
        static const size_t timestamp_query_count = 1024;
        static const size_t occlusion_query_count = 1024;
        bool initial_querypool_reset = false;
        std::vector<uint32_t> timestamps_to_reset;
        std::vector<uint32_t> occlusions_to_reset;

        void CreateBackBufferResources();

        std::mutex copyQueueLock;
        bool copyQueueUse = false;
        VkSemaphore copySemaphore = VK_NULL_HANDLE;

        struct FrameResources
        {
            VkFence frameFence = VK_NULL_HANDLE;

            VkQueue copyQueue = VK_NULL_HANDLE;
            VkCommandPool copyCommandPool = VK_NULL_HANDLE;
            VkCommandBuffer copyCommandBuffer = VK_NULL_HANDLE;

            VkCommandPool transitionCommandPool = VK_NULL_HANDLE;
            VkCommandBuffer transitionCommandBuffer = VK_NULL_HANDLE;
            std::vector<VkImageMemoryBarrier> loadedimagetransitions;

            VkSemaphore swapchainAcquireSemaphore = VK_NULL_HANDLE;
            VkSemaphore swapchainReleaseSemaphore = VK_NULL_HANDLE;
        };
        FrameResources frames[BACKBUFFER_COUNT];
        FrameResources& GetFrameResources()
        {
            return frames[GetFrameIndex()];
        }

        std::unordered_map<size_t, VkPipeline> pipelines_global;

        Vulkan_CommandList* commandLists[kCommandListCount] = {};
        std::atomic_uint32_t commandListsCount{0};

        /// A set of semaphores that can be reused.
        std::vector<VkSemaphore> recycledSemaphores;

        static PFN_vkCreateRayTracingPipelinesKHR createRayTracingPipelinesKHR;
        static PFN_vkCreateAccelerationStructureKHR createAccelerationStructureKHR;
        static PFN_vkBindAccelerationStructureMemoryKHR bindAccelerationStructureMemoryKHR;
        static PFN_vkDestroyAccelerationStructureKHR destroyAccelerationStructureKHR;
        static PFN_vkGetAccelerationStructureMemoryRequirementsKHR getAccelerationStructureMemoryRequirementsKHR;
        static PFN_vkGetAccelerationStructureDeviceAddressKHR getAccelerationStructureDeviceAddressKHR;
        static PFN_vkGetRayTracingShaderGroupHandlesKHR getRayTracingShaderGroupHandlesKHR;
        static PFN_vkCmdBuildAccelerationStructureKHR cmdBuildAccelerationStructureKHR;
        static PFN_vkCmdTraceRaysKHR cmdTraceRaysKHR;

        static PFN_vkCmdDrawMeshTasksNV cmdDrawMeshTasksNV;
        static PFN_vkCmdDrawMeshTasksIndirectNV cmdDrawMeshTasksIndirectNV;

    public:
        static bool IsAvailable();

        GraphicsDevice_Vulkan(WindowHandle window, const GraphicsSettings& desc);
        virtual ~GraphicsDevice_Vulkan();

        VkDevice GetVkDevice() const
        {
            return device;
        }

        RefPtr<GraphicsBuffer> CreateBuffer(const GPUBufferDesc& desc, const void* initialData) override;
        bool CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture) override;
        bool CreateShader(ShaderStage stafe, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader) override;
        bool CreateShader(ShaderStage stage, const char* source, const char* entryPoint, Shader* pShader) override;
        RefPtr<Sampler> CreateSampler(const SamplerDescriptor* descriptor) override;
        bool CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery) override;
        bool CreateRenderPipelineCore(const RenderPipelineDescriptor* descriptor, RenderPipeline** renderPipeline) override;
        bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) override;
        bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) override;
        bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) override;
        bool CreateDescriptorTable(DescriptorTable* table) override;
        bool CreateRootSignature(RootSignature* rootsig) override;

        int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) override;
        int CreateSubresource(GraphicsBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size = ~0) override;

        void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) override;
        void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) override;
        void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource = -1, uint64_t offset = 0) override;
        void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler) override;

        void Map(const GPUResource* resource, Mapping* mapping) override;
        void Unmap(const GPUResource* resource) override;
        bool QueryRead(const GPUQuery* query, GPUQueryResult* result) override;

        void SetName(GPUResource* pResource, const char* name) override;

        VkSemaphore RequestSemaphore();
        void ReturnSemaphore(VkSemaphore semaphore);

        CommandList& BeginCommandList() override;
        void SubmitCommandLists() override;

        void WaitForGPU() override;
        void ClearPipelineStateCache() override;

        void Resize(uint32_t width, uint32_t height) override;

        Texture GetBackBuffer() override;

        void SetVSyncEnabled(bool value) override
        {
            verticalSync = value;
            CreateBackBufferResources();
        };

        struct AllocationHandler
        {
            VmaAllocator allocator = VK_NULL_HANDLE;
            VkDevice device = VK_NULL_HANDLE;
            VkInstance instance;
            uint64_t framecount = 0;
            std::mutex destroylocker;
            std::deque<std::pair<std::pair<VkImage, VmaAllocation>, uint64_t>> destroyer_images;
            std::deque<std::pair<VkImageView, uint64_t>> destroyer_imageviews;
            std::deque<std::pair<std::pair<VkBuffer, VmaAllocation>, uint64_t>> destroyer_buffers;
            std::deque<std::pair<VkBufferView, uint64_t>> destroyer_bufferviews;
            std::deque<std::pair<VkAccelerationStructureKHR, uint64_t>> destroyer_bvhs;
            std::deque<std::pair<VkSampler, uint64_t>> destroyer_samplers;
            std::deque<std::pair<VkDescriptorPool, uint64_t>> destroyer_descriptorPools;
            std::deque<std::pair<VkDescriptorSetLayout, uint64_t>> destroyer_descriptorSetLayouts;
            std::deque<std::pair<VkDescriptorUpdateTemplate, uint64_t>> destroyer_descriptorUpdateTemplates;
            std::deque<std::pair<VkShaderModule, uint64_t>> destroyer_shadermodules;
            std::deque<std::pair<VkPipelineLayout, uint64_t>> destroyer_pipelineLayouts;
            std::deque<std::pair<VkPipeline, uint64_t>> destroyer_pipelines;
            std::deque<std::pair<VkRenderPass, uint64_t>> destroyer_renderpasses;
            std::deque<std::pair<VkFramebuffer, uint64_t>> destroyer_framebuffers;
            std::deque<std::pair<uint32_t, uint64_t>> destroyer_queries_occlusion;
            std::deque<std::pair<uint32_t, uint64_t>> destroyer_queries_timestamp;

            ThreadSafeRingBuffer<uint32_t, timestamp_query_count> free_timestampqueries;
            ThreadSafeRingBuffer<uint32_t, occlusion_query_count> free_occlusionqueries;

            ~AllocationHandler()
            {
            }

            // Deferred destroy of resources that the GPU is already finished with:
            void Update(uint64_t FRAMECOUNT, uint32_t BACKBUFFER_COUNT)
            {
                destroylocker.lock();
                framecount = FRAMECOUNT;
                while (!destroyer_images.empty())
                {
                    if (destroyer_images.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_images.front();
                        destroyer_images.pop_front();
                        vmaDestroyImage(allocator, item.first.first, item.first.second);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_imageviews.empty())
                {
                    if (destroyer_imageviews.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_imageviews.front();
                        destroyer_imageviews.pop_front();
                        vkDestroyImageView(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_buffers.empty())
                {
                    if (destroyer_buffers.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_buffers.front();
                        destroyer_buffers.pop_front();
                        vmaDestroyBuffer(allocator, item.first.first, item.first.second);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_bufferviews.empty())
                {
                    if (destroyer_bufferviews.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_bufferviews.front();
                        destroyer_bufferviews.pop_front();
                        vkDestroyBufferView(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_bvhs.empty())
                {
                    if (destroyer_bvhs.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_bvhs.front();
                        destroyer_bvhs.pop_front();
                        destroyAccelerationStructureKHR(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_samplers.empty())
                {
                    if (destroyer_samplers.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_samplers.front();
                        destroyer_samplers.pop_front();
                        vkDestroySampler(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_descriptorPools.empty())
                {
                    if (destroyer_descriptorPools.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_descriptorPools.front();
                        destroyer_descriptorPools.pop_front();
                        vkDestroyDescriptorPool(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_descriptorSetLayouts.empty())
                {
                    if (destroyer_descriptorSetLayouts.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_descriptorSetLayouts.front();
                        destroyer_descriptorSetLayouts.pop_front();
                        vkDestroyDescriptorSetLayout(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_descriptorUpdateTemplates.empty())
                {
                    if (destroyer_descriptorUpdateTemplates.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_descriptorUpdateTemplates.front();
                        destroyer_descriptorUpdateTemplates.pop_front();
                        vkDestroyDescriptorUpdateTemplate(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_shadermodules.empty())
                {
                    if (destroyer_shadermodules.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_shadermodules.front();
                        destroyer_shadermodules.pop_front();
                        vkDestroyShaderModule(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_pipelineLayouts.empty())
                {
                    if (destroyer_pipelineLayouts.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_pipelineLayouts.front();
                        destroyer_pipelineLayouts.pop_front();
                        vkDestroyPipelineLayout(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_pipelines.empty())
                {
                    if (destroyer_pipelines.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_pipelines.front();
                        destroyer_pipelines.pop_front();
                        vkDestroyPipeline(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_renderpasses.empty())
                {
                    if (destroyer_renderpasses.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_renderpasses.front();
                        destroyer_renderpasses.pop_front();
                        vkDestroyRenderPass(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_framebuffers.empty())
                {
                    if (destroyer_framebuffers.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_framebuffers.front();
                        destroyer_framebuffers.pop_front();
                        vkDestroyFramebuffer(device, item.first, nullptr);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_queries_occlusion.empty())
                {
                    if (destroyer_queries_occlusion.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_queries_occlusion.front();
                        destroyer_queries_occlusion.pop_front();
                        free_occlusionqueries.push_back(item.first);
                    }
                    else
                    {
                        break;
                    }
                }
                while (!destroyer_queries_timestamp.empty())
                {
                    if (destroyer_queries_timestamp.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
                    {
                        auto item = destroyer_queries_timestamp.front();
                        destroyer_queries_timestamp.pop_front();
                        free_timestampqueries.push_back(item.first);
                    }
                    else
                    {
                        break;
                    }
                }
                destroylocker.unlock();
            }
        };

        std::shared_ptr<AllocationHandler> allocationhandler;
    };

    PFN_vkCreateRayTracingPipelinesKHR GraphicsDevice_Vulkan::createRayTracingPipelinesKHR = nullptr;
    PFN_vkCreateAccelerationStructureKHR GraphicsDevice_Vulkan::createAccelerationStructureKHR = nullptr;
    PFN_vkBindAccelerationStructureMemoryKHR GraphicsDevice_Vulkan::bindAccelerationStructureMemoryKHR = nullptr;
    PFN_vkDestroyAccelerationStructureKHR GraphicsDevice_Vulkan::destroyAccelerationStructureKHR = nullptr;
    PFN_vkGetAccelerationStructureMemoryRequirementsKHR GraphicsDevice_Vulkan::getAccelerationStructureMemoryRequirementsKHR = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR GraphicsDevice_Vulkan::getAccelerationStructureDeviceAddressKHR = nullptr;
    PFN_vkGetRayTracingShaderGroupHandlesKHR GraphicsDevice_Vulkan::getRayTracingShaderGroupHandlesKHR = nullptr;
    PFN_vkCmdBuildAccelerationStructureKHR GraphicsDevice_Vulkan::cmdBuildAccelerationStructureKHR = nullptr;
    PFN_vkCmdTraceRaysKHR GraphicsDevice_Vulkan::cmdTraceRaysKHR = nullptr;

    PFN_vkCmdDrawMeshTasksNV GraphicsDevice_Vulkan::cmdDrawMeshTasksNV = nullptr;
    PFN_vkCmdDrawMeshTasksIndirectNV GraphicsDevice_Vulkan::cmdDrawMeshTasksIndirectNV = nullptr;

    namespace Vulkan_Internal
    {
        // Converters:
        constexpr VkFormat _ConvertFormat(PixelFormat value)
        {
            switch (value)
            {
                case PixelFormat::Invalid:
                    return VK_FORMAT_UNDEFINED;
                    // 8-bit formats
                case PixelFormat::R8Unorm:
                    return VK_FORMAT_R8_UNORM;
                case PixelFormat::R8Snorm:
                    return VK_FORMAT_R8_SNORM;
                case PixelFormat::R8Uint:
                    return VK_FORMAT_R8_UINT;
                case PixelFormat::R8Sint:
                    return VK_FORMAT_R8_SINT;

                    /*case PixelFormat::FORMAT_R32G32B32A32_FLOAT:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            case PixelFormat::FORMAT_R32G32B32A32_UINT:
                return VK_FORMAT_R32G32B32A32_UINT;
                break;
            case PixelFormat::FORMAT_R32G32B32A32_SINT:
                return VK_FORMAT_R32G32B32A32_SINT;
                break;
            case PixelFormat::FORMAT_R32G32B32_FLOAT:
                return VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case PixelFormat::FORMAT_R32G32B32_UINT:
                return VK_FORMAT_R32G32B32_UINT;
                break;
            case PixelFormat::FORMAT_R32G32B32_SINT:
                return VK_FORMAT_R32G32B32_SINT;
                break;
            case PixelFormat::FORMAT_R16G16B16A16_FLOAT:
                return VK_FORMAT_R16G16B16A16_SFLOAT;
                break;
            case PixelFormat::FORMAT_R16G16B16A16_UNORM:
                return VK_FORMAT_R16G16B16A16_UNORM;
                break;
            case PixelFormat::FORMAT_R16G16B16A16_UINT:
                return VK_FORMAT_R16G16B16A16_UINT;
                break;
            case PixelFormat::FORMAT_R16G16B16A16_SNORM:
                return VK_FORMAT_R16G16B16A16_SNORM;
                break;
            case PixelFormat::FORMAT_R16G16B16A16_SINT:
                return VK_FORMAT_R16G16B16A16_SINT;
            case PixelFormat::FORMAT_R32G32_FLOAT:
                return VK_FORMAT_R32G32_SFLOAT;
            case PixelFormat::FORMAT_R32G32_UINT:
                return VK_FORMAT_R32G32_UINT;
            case PixelFormat::FORMAT_R32G32_SINT:
                return VK_FORMAT_R32G32_SINT;
            case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
                break;
            case PixelFormat::FORMAT_D32_FLOAT_S8X24_UINT:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
                break;
            case PixelFormat::FORMAT_R10G10B10A2_UNORM:
                return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
                break;
            case PixelFormat::FORMAT_R10G10B10A2_UINT:
                return VK_FORMAT_A2B10G10R10_UINT_PACK32;
                break;
            case PixelFormat::FORMAT_R11G11B10_FLOAT:
                return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
                break;
            case PixelFormat::FORMAT_R8G8B8A8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
                break;
            case PixelFormat::FORMAT_R8G8B8A8_UNORM_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;
                break;
            case PixelFormat::FORMAT_R8G8B8A8_UINT:
                return VK_FORMAT_R8G8B8A8_UINT;
                break;
            case PixelFormat::FORMAT_R8G8B8A8_SNORM:
                return VK_FORMAT_R8G8B8A8_SNORM;
                break;
            case PixelFormat::FORMAT_R8G8B8A8_SINT:
                return VK_FORMAT_R8G8B8A8_SINT;
                break;
            case PixelFormat::FORMAT_R16G16_FLOAT:
                return VK_FORMAT_R16G16_SFLOAT;
                break;
            case PixelFormat::FORMAT_R16G16_UNORM:
                return VK_FORMAT_R16G16_UNORM;
                break;
            case PixelFormat::FORMAT_R16G16_UINT:
                return VK_FORMAT_R16G16_UINT;
                break;
            case PixelFormat::FORMAT_R16G16_SNORM:
                return VK_FORMAT_R16G16_SNORM;
                break;
            case PixelFormat::FORMAT_R16G16_SINT:
                return VK_FORMAT_R16G16_SINT;
                break;
            case PixelFormat::FORMAT_R32_TYPELESS:
                return VK_FORMAT_D32_SFLOAT;
                break;
            case PixelFormat::FORMAT_D32_FLOAT:
                return VK_FORMAT_D32_SFLOAT;
                break;
            case PixelFormat::FORMAT_R32_FLOAT:
                return VK_FORMAT_R32_SFLOAT;
                break;
            case PixelFormat::FORMAT_R32_UINT:
                return VK_FORMAT_R32_UINT;
                break;
            case PixelFormat::FORMAT_R32_SINT:
                return VK_FORMAT_R32_SINT;
                break;
            case PixelFormat::FORMAT_R24G8_TYPELESS:
                return VK_FORMAT_D24_UNORM_S8_UINT;
                break;
            case PixelFormat::FORMAT_D24_UNORM_S8_UINT:
                return VK_FORMAT_D24_UNORM_S8_UINT;
                break;
            case PixelFormat::FORMAT_R8G8_UNORM:
                return VK_FORMAT_R8G8_UNORM;
                break;
            case PixelFormat::FORMAT_R8G8_UINT:
                return VK_FORMAT_R8G8_UINT;
                break;
            case PixelFormat::FORMAT_R8G8_SNORM:
                return VK_FORMAT_R8G8_SNORM;
                break;
            case PixelFormat::FORMAT_R8G8_SINT:
                return VK_FORMAT_R8G8_SINT;
                break;
            case PixelFormat::FORMAT_R16_TYPELESS:
                return VK_FORMAT_D16_UNORM;
                break;
            case PixelFormat::FORMAT_R16_FLOAT:
                return VK_FORMAT_R16_SFLOAT;
                break;
            case PixelFormat::FORMAT_D16_UNORM:
                return VK_FORMAT_D16_UNORM;
                break;
            case PixelFormat::FORMAT_R16_UNORM:
                return VK_FORMAT_R16_UNORM;
                break;
            case PixelFormat::FORMAT_R16_UINT:
                return VK_FORMAT_R16_UINT;
                break;
            case PixelFormat::FORMAT_R16_SNORM:
                return VK_FORMAT_R16_SNORM;
                break;
            case PixelFormat::FORMAT_R16_SINT:
                return VK_FORMAT_R16_SINT;
            case PixelFormat::FORMAT_BC1_UNORM:
                return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
                break;
            case PixelFormat::FORMAT_BC1_UNORM_SRGB:
                return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
                break;
            case PixelFormat::FORMAT_BC2_UNORM:
                return VK_FORMAT_BC2_UNORM_BLOCK;
                break;
            case PixelFormat::FORMAT_BC2_UNORM_SRGB:
                return VK_FORMAT_BC2_SRGB_BLOCK;
                break;
            case PixelFormat::FORMAT_BC3_UNORM:
                return VK_FORMAT_BC3_UNORM_BLOCK;
                break;
            case PixelFormat::FORMAT_BC3_UNORM_SRGB:
                return VK_FORMAT_BC3_SRGB_BLOCK;
                break;
            case PixelFormat::FORMAT_BC4_UNORM:
                return VK_FORMAT_BC4_UNORM_BLOCK;
                break;
            case PixelFormat::FORMAT_BC4_SNORM:
                return VK_FORMAT_BC4_SNORM_BLOCK;
                break;
            case PixelFormat::FORMAT_BC5_UNORM:
                return VK_FORMAT_BC5_UNORM_BLOCK;
                break;
            case PixelFormat::FORMAT_BC5_SNORM:
                return VK_FORMAT_BC5_SNORM_BLOCK;
                break;
            case PixelFormat::FORMAT_B8G8R8A8_UNORM:
                return VK_FORMAT_B8G8R8A8_UNORM;
                break;
            case PixelFormat::FORMAT_B8G8R8A8_UNORM_SRGB:
                return VK_FORMAT_B8G8R8A8_SRGB;
                break;
            case PixelFormat::FORMAT_BC6H_UF16:
                return VK_FORMAT_BC6H_UFLOAT_BLOCK;
                break;
            case PixelFormat::FORMAT_BC6H_SF16:
                return VK_FORMAT_BC6H_SFLOAT_BLOCK;
            case PixelFormat::FORMAT_BC7_UNORM:
                return VK_FORMAT_BC7_UNORM_BLOCK;
            case PixelFormat::FORMAT_BC7_UNORM_SRGB:
                return VK_FORMAT_BC7_SRGB_BLOCK;*/
                default:
                    ALIMER_UNREACHABLE();
            }

            return VK_FORMAT_UNDEFINED;
        }

        constexpr VkFormat _ConvertVertexFormat(VertexFormat format)
        {
            switch (format)
            {
                case VertexFormat::UChar2:
                    return VK_FORMAT_R8G8_UINT;
                case VertexFormat::UChar4:
                    return VK_FORMAT_R8G8B8A8_UINT;
                case VertexFormat::Char2:
                    return VK_FORMAT_R8G8_SINT;
                case VertexFormat::Char4:
                    return VK_FORMAT_R8G8B8A8_SINT;
                case VertexFormat::UChar2Norm:
                    return VK_FORMAT_R8G8_UNORM;
                case VertexFormat::UChar4Norm:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case VertexFormat::Char2Norm:
                    return VK_FORMAT_R8G8_SNORM;
                case VertexFormat::Char4Norm:
                    return VK_FORMAT_R8G8B8A8_SNORM;
                case VertexFormat::UShort2:
                    return VK_FORMAT_R16G16_UINT;
                case VertexFormat::UShort4:
                    return VK_FORMAT_R16G16B16A16_UINT;
                case VertexFormat::Short2:
                    return VK_FORMAT_R16G16_SINT;
                case VertexFormat::Short4:
                    return VK_FORMAT_R16G16B16A16_SINT;
                case VertexFormat::UShort2Norm:
                    return VK_FORMAT_R16G16_UNORM;
                case VertexFormat::UShort4Norm:
                    return VK_FORMAT_R16G16B16A16_UNORM;
                case VertexFormat::Short2Norm:
                    return VK_FORMAT_R16G16_SNORM;
                case VertexFormat::Short4Norm:
                    return VK_FORMAT_R16G16B16A16_SNORM;
                case VertexFormat::Half2:
                    return VK_FORMAT_R16G16_SFLOAT;
                case VertexFormat::Half4:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case VertexFormat::Float:
                    return VK_FORMAT_R32_SFLOAT;
                case VertexFormat::Float2:
                    return VK_FORMAT_R32G32_SFLOAT;
                case VertexFormat::Float3:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case VertexFormat::Float4:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case VertexFormat::UInt:
                    return VK_FORMAT_R32_UINT;
                case VertexFormat::UInt2:
                    return VK_FORMAT_R32G32_UINT;
                case VertexFormat::UInt3:
                    return VK_FORMAT_R32G32B32_UINT;
                case VertexFormat::UInt4:
                    return VK_FORMAT_R32G32B32A32_UINT;
                case VertexFormat::Int:
                    return VK_FORMAT_R32_SINT;
                case VertexFormat::Int2:
                    return VK_FORMAT_R32G32_SINT;
                case VertexFormat::Int3:
                    return VK_FORMAT_R32G32B32_SINT;
                case VertexFormat::Int4:
                    return VK_FORMAT_R32G32B32A32_SINT;
                default:
                    ALIMER_UNREACHABLE();
            }
        }

        constexpr VkCompareOp _ConvertComparisonFunc(CompareFunction value)
        {
            switch (value)
            {
                case CompareFunction::Never:
                    return VK_COMPARE_OP_NEVER;
                case CompareFunction::Less:
                    return VK_COMPARE_OP_LESS;
                case CompareFunction::Equal:
                    return VK_COMPARE_OP_EQUAL;
                case CompareFunction::LessEqual:
                    return VK_COMPARE_OP_LESS_OR_EQUAL;
                case CompareFunction::Greater:
                    return VK_COMPARE_OP_GREATER;
                case CompareFunction::NotEqual:
                    return VK_COMPARE_OP_NOT_EQUAL;
                case CompareFunction::GreaterEqual:
                    return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case CompareFunction::Always:
                    return VK_COMPARE_OP_ALWAYS;
                default:
                    ALIMER_UNREACHABLE();
                    break;
            }
            return VK_COMPARE_OP_NEVER;
        }
        constexpr VkBlendFactor _ConvertBlend(BlendFactor value)
        {
            switch (value)
            {
                case BlendFactor::Zero:
                    return VK_BLEND_FACTOR_ZERO;
                case BlendFactor::One:
                    return VK_BLEND_FACTOR_ONE;
                case BlendFactor::SourceColor:
                    return VK_BLEND_FACTOR_SRC_COLOR;
                case BlendFactor::OneMinusSourceColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case BlendFactor::SourceAlpha:
                    return VK_BLEND_FACTOR_SRC_ALPHA;
                case BlendFactor::OneMinusSourceAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case BlendFactor::DestinationColor:
                    return VK_BLEND_FACTOR_DST_COLOR;
                case BlendFactor::OneMinusDestinationColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case BlendFactor::DestinationAlpha:
                    return VK_BLEND_FACTOR_DST_ALPHA;
                case BlendFactor::OneMinusDestinationAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case BlendFactor::SourceAlphaSaturated:
                    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                case BlendFactor::BlendColor:
                    return VK_BLEND_FACTOR_CONSTANT_COLOR;
                case BlendFactor::OneMinusBlendColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                    //case BlendFactor::BlendAlpha:
                    //    return VK_BLEND_FACTOR_CONSTANT_ALPHA;
                    //case BlendFactor::OneMinusBlendAlpha:
                    //    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
                case BlendFactor::Source1Color:
                    return VK_BLEND_FACTOR_SRC1_COLOR;
                case BlendFactor::OneMinusSource1Color:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
                case BlendFactor::Source1Alpha:
                    return VK_BLEND_FACTOR_SRC1_ALPHA;
                case BlendFactor::OneMinusSource1Alpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
                default:
                    ALIMER_UNREACHABLE();
                    return VK_BLEND_FACTOR_ZERO;
            }
        }
        constexpr VkBlendOp _ConvertBlendOp(BlendOperation value)
        {
            switch (value)
            {
                case BlendOperation::Add:
                    return VK_BLEND_OP_ADD;
                case BlendOperation::Subtract:
                    return VK_BLEND_OP_SUBTRACT;
                case BlendOperation::ReverseSubtract:
                    return VK_BLEND_OP_REVERSE_SUBTRACT;
                case BlendOperation::Min:
                    return VK_BLEND_OP_MIN;
                case BlendOperation::Max:
                    return VK_BLEND_OP_MAX;
                    break;
                default:
                    ALIMER_UNREACHABLE();
                    return VK_BLEND_OP_ADD;
            }
        }
        constexpr VkFilter _ConvertFilter(FilterMode filter)
        {
            switch (filter)
            {
                case FilterMode::Nearest:
                    return VK_FILTER_NEAREST;
                case FilterMode::Linear:
                    return VK_FILTER_LINEAR;
                default:
                    ALIMER_UNREACHABLE();
                    return VK_FILTER_NEAREST;
            }
        }
        VkSamplerMipmapMode _ConvertMipMapFilterMode(FilterMode filter)
        {
            switch (filter)
            {
                case FilterMode::Nearest:
                    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
                case FilterMode::Linear:
                    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
                default:
                    ALIMER_UNREACHABLE();
                    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            }
        }

        constexpr VkSamplerAddressMode _ConvertAddressMode(SamplerAddressMode value)
        {
            switch (value)
            {
                case SamplerAddressMode::Wrap:
                    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case SamplerAddressMode::Mirror:
                    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                case SamplerAddressMode::Clamp:
                    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case SamplerAddressMode::Border:
                    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                default:
                    ALIMER_UNREACHABLE();
                    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            }
        }
        constexpr VkBorderColor _ConvertSamplerBorderColor(SamplerBorderColor value)
        {
            switch (value)
            {
                case SamplerBorderColor::TransparentBlack:
                    return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
                case SamplerBorderColor::OpaqueBlack:
                    return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                case SamplerBorderColor::OpaqueWhite:
                    return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                default:
                    ALIMER_UNREACHABLE();
                    return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            }
        }
        constexpr VkStencilOp _ConvertStencilOp(StencilOperation value)
        {
            switch (value)
            {
                case StencilOperation::Keep:
                    return VK_STENCIL_OP_KEEP;
                case StencilOperation::Zero:
                    return VK_STENCIL_OP_ZERO;
                case StencilOperation::Replace:
                    return VK_STENCIL_OP_REPLACE;
                case StencilOperation::IncrementClamp:
                    return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
                case StencilOperation::DecrementClamp:
                    return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
                case StencilOperation::Invert:
                    return VK_STENCIL_OP_INVERT;
                case StencilOperation::IncrementWrap:
                    return VK_STENCIL_OP_INCREMENT_AND_WRAP;
                case StencilOperation::DecrementWrap:
                    return VK_STENCIL_OP_DECREMENT_AND_WRAP;
                default:
                    ALIMER_UNREACHABLE();
                    break;
            }
            return VK_STENCIL_OP_KEEP;
        }
        constexpr VkImageLayout _ConvertImageLayout(IMAGE_LAYOUT value)
        {
            switch (value)
            {
                case alimer::IMAGE_LAYOUT_GENERAL:
                    return VK_IMAGE_LAYOUT_GENERAL;
                case alimer::IMAGE_LAYOUT_RENDERTARGET:
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                case alimer::IMAGE_LAYOUT_DEPTHSTENCIL:
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                case alimer::IMAGE_LAYOUT_DEPTHSTENCIL_READONLY:
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                case alimer::IMAGE_LAYOUT_SHADER_RESOURCE:
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                case alimer::IMAGE_LAYOUT_UNORDERED_ACCESS:
                    return VK_IMAGE_LAYOUT_GENERAL;
                case alimer::IMAGE_LAYOUT_COPY_SRC:
                    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                case alimer::IMAGE_LAYOUT_COPY_DST:
                    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
        constexpr VkShaderStageFlags _ConvertStageFlags(ShaderStage value)
        {
            switch (value)
            {
                case ShaderStage::Vertex:
                    return VK_SHADER_STAGE_VERTEX_BIT;
                case ShaderStage::Hull:
                    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                case ShaderStage::Domain:
                    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                case ShaderStage::Geometry:
                    return VK_SHADER_STAGE_GEOMETRY_BIT;
                case ShaderStage::Fragment:
                    return VK_SHADER_STAGE_FRAGMENT_BIT;
                case ShaderStage::Compute:
                    return VK_SHADER_STAGE_COMPUTE_BIT;
                case ShaderStage::Amplification:
                    return VK_SHADER_STAGE_TASK_BIT_NV;
                case ShaderStage::Mesh:
                    return VK_SHADER_STAGE_MESH_BIT_NV;
                default:
                case ShaderStage::Count:
                    return VK_SHADER_STAGE_ALL;
            }
        }

        inline VkAccessFlags _ParseImageLayout(IMAGE_LAYOUT value)
        {
            VkAccessFlags flags = 0;

            switch (value)
            {
                case alimer::IMAGE_LAYOUT_GENERAL:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_SHADER_WRITE_BIT;
                    flags |= VK_ACCESS_TRANSFER_READ_BIT;
                    flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
                    flags |= VK_ACCESS_MEMORY_READ_BIT;
                    flags |= VK_ACCESS_MEMORY_WRITE_BIT;
                    break;
                case alimer::IMAGE_LAYOUT_RENDERTARGET:
                    flags |= VK_ACCESS_SHADER_WRITE_BIT;
                    break;
                case alimer::IMAGE_LAYOUT_DEPTHSTENCIL:
                    flags |= VK_ACCESS_SHADER_WRITE_BIT;
                    break;
                case alimer::IMAGE_LAYOUT_DEPTHSTENCIL_READONLY:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    break;
                case alimer::IMAGE_LAYOUT_SHADER_RESOURCE:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    break;
                case alimer::IMAGE_LAYOUT_UNORDERED_ACCESS:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_SHADER_WRITE_BIT;
                    break;
                case alimer::IMAGE_LAYOUT_COPY_SRC:
                    flags |= VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case alimer::IMAGE_LAYOUT_COPY_DST:
                    flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
            }

            return flags;
        }
        inline VkAccessFlags _ParseBufferState(BUFFER_STATE value)
        {
            VkAccessFlags flags = 0;

            switch (value)
            {
                case alimer::BUFFER_STATE_GENERAL:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_SHADER_WRITE_BIT;
                    flags |= VK_ACCESS_TRANSFER_READ_BIT;
                    flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
                    flags |= VK_ACCESS_HOST_READ_BIT;
                    flags |= VK_ACCESS_HOST_WRITE_BIT;
                    flags |= VK_ACCESS_MEMORY_READ_BIT;
                    flags |= VK_ACCESS_MEMORY_WRITE_BIT;
                    flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                    flags |= VK_ACCESS_INDEX_READ_BIT;
                    flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                    flags |= VK_ACCESS_UNIFORM_READ_BIT;
                    break;
                case alimer::BUFFER_STATE_VERTEX_BUFFER:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                    break;
                case alimer::BUFFER_STATE_INDEX_BUFFER:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_INDEX_READ_BIT;
                    break;
                case alimer::BUFFER_STATE_CONSTANT_BUFFER:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_UNIFORM_READ_BIT;
                    break;
                case alimer::BUFFER_STATE_INDIRECT_ARGUMENT:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                    break;
                case alimer::BUFFER_STATE_SHADER_RESOURCE:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_UNIFORM_READ_BIT;
                    break;
                case alimer::BUFFER_STATE_UNORDERED_ACCESS:
                    flags |= VK_ACCESS_SHADER_READ_BIT;
                    flags |= VK_ACCESS_SHADER_WRITE_BIT;
                    break;
                case alimer::BUFFER_STATE_COPY_SRC:
                    flags |= VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case alimer::BUFFER_STATE_COPY_DST:
                    flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                default:
                    break;
            }

            return flags;
        }

        // Extension functions:
        bool checkDeviceExtensionSupport(const char* checkExtension, const std::vector<VkExtensionProperties>& available_deviceExtensions)
        {
            for (const auto& x : available_deviceExtensions)
            {
                if (strcmp(x.extensionName, checkExtension) == 0)
                {
                    return true;
                }
            }

            return false;
        }

        // Validation layer helpers:
        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"};
        bool checkValidationLayerSupport()
        {
            uint32_t layerCount;
            VkResult res = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            assert(res == VK_SUCCESS);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            res = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
            assert(res == VK_SUCCESS);

            for (const char* layerName : validationLayers)
            {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers)
                {
                    if (strcmp(layerName, layerProperties.layerName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound)
                {
                    return false;
                }
            }

            return true;
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
        {
            // Log debug messge
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                LOGW("{} - {}: {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
            }
            else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                LOGE("{} - {}: {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
            }

            return VK_FALSE;
        }

        // Queue families:
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            QueueFamilyIndices indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies)
            {
                VkBool32 presentSupport = false;
                VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                assert(res == VK_SUCCESS);

                if (indices.presentFamily < 0 && queueFamily.queueCount > 0 && presentSupport)
                {
                    indices.presentFamily = i;
                }

                if (indices.graphicsFamily < 0 && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    indices.graphicsFamily = i;
                }

                if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    indices.copyFamily = i;
                }

                i++;
            }

            return indices;
        }

        // Swapchain helpers:
        struct SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            SwapChainSupportDetails details;

            VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
            assert(res == VK_SUCCESS);

            uint32_t formatCount;
            res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
            assert(res == VK_SUCCESS);

            if (formatCount != 0)
            {
                details.formats.resize(formatCount);
                res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
                assert(res == VK_SUCCESS);
            }

            uint32_t presentModeCount;
            res = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
            assert(res == VK_SUCCESS);

            if (presentModeCount != 0)
            {
                details.presentModes.resize(presentModeCount);
                res = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
                assert(res == VK_SUCCESS);
            }

            return details;
        }

        uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }

            assert(0);
            return ~0;
        }

        // Device selection helpers:
        const std::vector<const char*> required_deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
        };
        bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            QueueFamilyIndices indices = findQueueFamilies(device, surface);
            if (!indices.isComplete())
            {
                return false;
            }

            uint32_t extensionCount;
            VkResult res = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
            assert(res == VK_SUCCESS);
            std::vector<VkExtensionProperties> available(extensionCount);
            res = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data());
            assert(res == VK_SUCCESS);

            for (auto& x : required_deviceExtensions)
            {
                if (!checkDeviceExtensionSupport(x, available))
                {
                    return false; // device doesn't have a required extension
                }
            }

            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);

            return !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // Memory tools:

        inline size_t Align(size_t uLocation, size_t uAlign)
        {
            if ((0 == uAlign) || (uAlign & (uAlign - 1)))
            {
                assert(0);
            }

            return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
        }

        // Destroyers:
        struct Buffer_Vulkan : public GraphicsBuffer
        {
            Buffer_Vulkan(const GPUBufferDesc& desc_)
                : GraphicsBuffer(desc_)
            {
            }

            ~Buffer_Vulkan() override
            {
                Destroy();
            }

            void Destroy() override
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (resource)
                    allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
                if (cbv)
                    allocationhandler->destroyer_bufferviews.push_back(std::make_pair(cbv, framecount));
                if (srv)
                    allocationhandler->destroyer_bufferviews.push_back(std::make_pair(srv, framecount));
                if (uav)
                    allocationhandler->destroyer_bufferviews.push_back(std::make_pair(uav, framecount));
                for (auto x : subresources_srv)
                {
                    allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x, framecount));
                }
                for (auto x : subresources_uav)
                {
                    allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x, framecount));
                }
                allocationhandler->destroylocker.unlock();
            }

            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VmaAllocation allocation = nullptr;
            VkBuffer resource = VK_NULL_HANDLE;
            VkBufferView cbv = VK_NULL_HANDLE;
            VkBufferView srv = VK_NULL_HANDLE;
            VkBufferView uav = VK_NULL_HANDLE;
            std::vector<VkBufferView> subresources_srv;
            std::vector<VkBufferView> subresources_uav;

            GPUAllocation dynamic[kCommandListCount];
        };

        struct Texture_Vulkan
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VmaAllocation allocation = nullptr;
            VkImage resource = VK_NULL_HANDLE;
            VkBuffer staging_resource = VK_NULL_HANDLE;
            VkImageView srv = VK_NULL_HANDLE;
            VkImageView uav = VK_NULL_HANDLE;
            VkImageView rtv = VK_NULL_HANDLE;
            VkImageView dsv = VK_NULL_HANDLE;
            std::vector<VkImageView> subresources_srv;
            std::vector<VkImageView> subresources_uav;
            std::vector<VkImageView> subresources_rtv;
            std::vector<VkImageView> subresources_dsv;

            VkSubresourceLayout subresourcelayout = {};

            ~Texture_Vulkan()
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (resource)
                    allocationhandler->destroyer_images.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
                if (staging_resource)
                    allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(staging_resource, allocation), framecount));
                if (srv)
                    allocationhandler->destroyer_imageviews.push_back(std::make_pair(srv, framecount));
                if (uav)
                    allocationhandler->destroyer_imageviews.push_back(std::make_pair(uav, framecount));
                if (srv)
                    allocationhandler->destroyer_imageviews.push_back(std::make_pair(rtv, framecount));
                if (uav)
                    allocationhandler->destroyer_imageviews.push_back(std::make_pair(dsv, framecount));
                for (auto x : subresources_srv)
                {
                    allocationhandler->destroyer_imageviews.push_back(std::make_pair(x, framecount));
                }
                for (auto x : subresources_uav)
                {
                    allocationhandler->destroyer_imageviews.push_back(std::make_pair(x, framecount));
                }
                for (auto x : subresources_rtv)
                {
                    allocationhandler->destroyer_imageviews.push_back(std::make_pair(x, framecount));
                }
                for (auto x : subresources_dsv)
                {
                    allocationhandler->destroyer_imageviews.push_back(std::make_pair(x, framecount));
                }
                allocationhandler->destroylocker.unlock();
            }
        };

        struct Sampler_Vulkan final : public Sampler
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VkSampler resource = VK_NULL_HANDLE;

            ~Sampler_Vulkan()
            {
                Destroy();
            }

            void Destroy() override
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (resource)
                    allocationhandler->destroyer_samplers.push_back(std::make_pair(resource, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };

        struct Query_Vulkan
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            GPU_QUERY_TYPE query_type = GPU_QUERY_TYPE_INVALID;
            uint32_t query_index = ~0;

            ~Query_Vulkan()
            {
                if (allocationhandler == nullptr)
                    return;
                if (query_index != ~0)
                {
                    allocationhandler->destroylocker.lock();
                    uint64_t framecount = allocationhandler->framecount;
                    switch (query_type)
                    {
                        case alimer::GPU_QUERY_TYPE_OCCLUSION:
                        case alimer::GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
                            allocationhandler->destroyer_queries_occlusion.push_back(std::make_pair(query_index, framecount));
                            break;
                        case alimer::GPU_QUERY_TYPE_TIMESTAMP:
                            allocationhandler->destroyer_queries_timestamp.push_back(std::make_pair(query_index, framecount));
                            break;
                    }
                    allocationhandler->destroylocker.unlock();
                }
            }
        };
        struct Shader_Vulkan
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VkShaderModule shaderModule = VK_NULL_HANDLE;
            VkPipeline pipeline_cs = VK_NULL_HANDLE;
            VkPipelineLayout pipelineLayout_cs = VK_NULL_HANDLE;
            VkPipelineShaderStageCreateInfo stageInfo = {};
            VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
            std::vector<VkImageViewType> imageViewTypes;

            std::vector<spirv_cross::EntryPoint> entrypoints;

            ~Shader_Vulkan()
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (shaderModule)
                    allocationhandler->destroyer_shadermodules.push_back(std::make_pair(shaderModule, framecount));
                if (pipeline_cs)
                    allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline_cs, framecount));
                if (pipelineLayout_cs)
                    allocationhandler->destroyer_pipelineLayouts.push_back(std::make_pair(pipelineLayout_cs, framecount));
                if (descriptorSetLayout)
                    allocationhandler->destroyer_descriptorSetLayouts.push_back(std::make_pair(descriptorSetLayout, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };
        struct PipelineState_Vulkan : public RenderPipeline
        {
            RenderPipelineDescriptor desc;
            size_t hash;

            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
            VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
            std::vector<VkImageViewType> imageViewTypes;

            ~PipelineState_Vulkan()
            {
                Destroy();
            }

            void Destroy() override
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (pipelineLayout)
                    allocationhandler->destroyer_pipelineLayouts.push_back(std::make_pair(pipelineLayout, framecount));
                if (descriptorSetLayout)
                    allocationhandler->destroyer_descriptorSetLayouts.push_back(std::make_pair(descriptorSetLayout, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };

        struct RenderPass_Vulkan
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VkRenderPass renderpass = VK_NULL_HANDLE;
            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            VkRenderPassBeginInfo beginInfo;
            VkClearValue clearColors[9] = {};

            ~RenderPass_Vulkan()
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (renderpass)
                    allocationhandler->destroyer_renderpasses.push_back(std::make_pair(renderpass, framecount));
                if (framebuffer)
                    allocationhandler->destroyer_framebuffers.push_back(std::make_pair(framebuffer, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };
        struct BVH_Vulkan
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VmaAllocation allocation = nullptr;
            VkBuffer buffer = VK_NULL_HANDLE;
            VkAccelerationStructureKHR resource = VK_NULL_HANDLE;

            VkAccelerationStructureCreateInfoKHR info = {};
            std::vector<VkAccelerationStructureCreateGeometryTypeInfoKHR> geometries;
            VkDeviceSize scratch_offset = 0;
            VkDeviceAddress as_address = 0;

            ~BVH_Vulkan()
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (buffer)
                    allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(buffer, allocation), framecount));
                if (resource)
                    allocationhandler->destroyer_bvhs.push_back(std::make_pair(resource, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };
        struct RTPipelineState_Vulkan
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VkPipeline pipeline;

            ~RTPipelineState_Vulkan()
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (pipeline)
                    allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };
        struct DescriptorTable_Vulkan
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VkDescriptorSetLayout layout = VK_NULL_HANDLE;
            VkDescriptorUpdateTemplate updatetemplate = VK_NULL_HANDLE;

            std::vector<size_t> resource_write_remap;
            std::vector<size_t> sampler_write_remap;

            struct Descriptor
            {
                union
                {
                    VkDescriptorImageInfo imageinfo;
                    VkDescriptorBufferInfo bufferInfo;
                    VkBufferView bufferView;
                    VkAccelerationStructureKHR accelerationStructure;
                };
            };
            std::vector<Descriptor> descriptors;

            ~DescriptorTable_Vulkan()
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (layout)
                    allocationhandler->destroyer_descriptorSetLayouts.push_back(std::make_pair(layout, framecount));
                if (updatetemplate)
                    allocationhandler->destroyer_descriptorUpdateTemplates.push_back(std::make_pair(updatetemplate, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };
        struct RootSignature_Vulkan
        {
            std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
            VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

            bool dirty[kCommandListCount] = {};
            std::vector<const DescriptorTable*> last_tables[kCommandListCount];
            std::vector<VkDescriptorSet> last_descriptorsets[kCommandListCount];
            std::vector<const GraphicsBuffer*> root_descriptors[kCommandListCount];
            std::vector<uint32_t> root_offsets[kCommandListCount];

            struct RootRemap
            {
                uint32_t space = 0;
                uint32_t binding = 0;
                uint32_t rangeIndex = 0;
            };
            std::vector<RootRemap> root_remap;

            ~RootSignature_Vulkan()
            {
                if (allocationhandler == nullptr)
                    return;
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (pipelineLayout)
                    allocationhandler->destroyer_pipelineLayouts.push_back(std::make_pair(pipelineLayout, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };

        Buffer_Vulkan* to_internal(GraphicsBuffer* param)
        {
            return static_cast<Buffer_Vulkan*>(param);
        }

        const Buffer_Vulkan* to_internal(const GraphicsBuffer* param)
        {
            return static_cast<const Buffer_Vulkan*>(param);
        }

        Texture_Vulkan* to_internal(const Texture* param)
        {
            return static_cast<Texture_Vulkan*>(param->internal_state.get());
        }
        const Sampler_Vulkan* to_internal(const Sampler* param)
        {
            return static_cast<const Sampler_Vulkan*>(param);
        }

        Query_Vulkan* to_internal(const GPUQuery* param)
        {
            return static_cast<Query_Vulkan*>(param->internal_state.get());
        }
        Shader_Vulkan* to_internal(const Shader* param)
        {
            return static_cast<Shader_Vulkan*>(param->internal_state.get());
        }

        const PipelineState_Vulkan* to_internal(const RenderPipeline* param)
        {
            return static_cast<const PipelineState_Vulkan*>(param);
        }

        RenderPass_Vulkan* to_internal(const RenderPass* param)
        {
            return static_cast<RenderPass_Vulkan*>(param->internal_state.get());
        }
        BVH_Vulkan* to_internal(const RaytracingAccelerationStructure* param)
        {
            return static_cast<BVH_Vulkan*>(param->internal_state.get());
        }
        RTPipelineState_Vulkan* to_internal(const RaytracingPipelineState* param)
        {
            return static_cast<RTPipelineState_Vulkan*>(param->internal_state.get());
        }
        DescriptorTable_Vulkan* to_internal(const DescriptorTable* param)
        {
            return static_cast<DescriptorTable_Vulkan*>(param->internal_state.get());
        }
        RootSignature_Vulkan* to_internal(const RootSignature* param)
        {
            return static_cast<RootSignature_Vulkan*>(param->internal_state.get());
        }

#if !defined(ALIMER_DISABLE_SHADER_COMPILER) && defined(VK_USE_PLATFORM_WIN32_KHR)
        using PFN_DXC_CREATE_INSTANCE = HRESULT(WINAPI*)(REFCLSID rclsid, REFIID riid, _COM_Outptr_ void** ppCompiler);

        PFN_DXC_CREATE_INSTANCE DxcCreateInstance;
        IDxcLibrary* dxcLibrary = nullptr;
        IDxcCompiler* dxcCompiler = nullptr;

        inline IDxcLibrary* GetOrCreateDxcLibrary()
        {
            if (DxcCreateInstance == nullptr)
            {
                static HMODULE dxcompilerDLL = LoadLibraryA("dxcompiler.dll");
                if (!dxcompilerDLL)
                {
                    return nullptr;
                }

                DxcCreateInstance = (PFN_DXC_CREATE_INSTANCE) GetProcAddress(dxcompilerDLL, "DxcCreateInstance");
            }

            if (dxcLibrary == nullptr)
            {
                DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&dxcLibrary));
                ALIMER_ASSERT(dxcLibrary != nullptr);
            }

            return dxcLibrary;
        }

        inline IDxcCompiler* GetOrCreateDxcCompiler()
        {
            if (DxcCreateInstance == nullptr)
            {
                static HMODULE dxcompilerDLL = LoadLibraryA("dxcompiler.dll");
                if (!dxcompilerDLL)
                {
                    return nullptr;
                }

                DxcCreateInstance = (PFN_DXC_CREATE_INSTANCE) GetProcAddress(dxcompilerDLL, "DxcCreateInstance");
            }

            if (dxcCompiler == nullptr)
            {
                DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
                ALIMER_ASSERT(dxcCompiler != nullptr);
            }

            return dxcCompiler;
        }
#endif
    }

    using namespace Vulkan_Internal;

    // Allocators:

    void ResourceFrameAllocator::init(GraphicsDevice_Vulkan* device, size_t size)
    {
        this->device = device;

        GPUBufferDesc bufferDesc = {};
        bufferDesc.ByteWidth = (uint32_t)(size);
        bufferDesc.Usage = USAGE_DYNAMIC;
        bufferDesc.BindFlags = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
        bufferDesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

        Buffer_Vulkan* newBuffer = new Buffer_Vulkan(bufferDesc);
        newBuffer->allocationhandler = device->allocationhandler;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bufferInfo.flags = 0;

        VkResult res;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        res = vmaCreateBuffer(device->allocationhandler->allocator, &bufferInfo, &allocInfo, &newBuffer->resource, &newBuffer->allocation, nullptr);
        assert(res == VK_SUCCESS);

        void* pData = newBuffer->allocation->GetMappedData();
        dataCur = dataBegin = reinterpret_cast<uint8_t*>(pData);
        dataEnd = dataBegin + size;

        // Because the "buffer" is created by hand in this, fill the desc to indicate how it can be used:
        ALIMER_ASSERT(bufferDesc.ByteWidth == (uint32_t)((size_t) dataEnd - (size_t) dataBegin));
        buffer.Reset(newBuffer);
    }

    uint8_t* ResourceFrameAllocator::Allocate(const uint32_t size, VkDeviceSize alignment)
    {
        dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

        if (dataCur + size > dataEnd)
        {
            init(device, ((size_t) dataEnd + size - (size_t) dataBegin) * 2);
        }

        uint8_t* retVal = dataCur;
        dataCur += size;
        return retVal;
    }

    void ResourceFrameAllocator::clear()
    {
        dataCur = dataBegin;
    }

    uint64_t ResourceFrameAllocator::calculateOffset(uint8_t* address)
    {
        assert(address >= dataBegin && address < dataEnd);
        return static_cast<uint64_t>(address - dataBegin);
    }

    /* DescriptorTableFrameAllocator */
    void DescriptorTableFrameAllocator::init(GraphicsDevice_Vulkan* device_)
    {
        device = device_;

        // Important that these don't reallocate themselves during writing dexcriptors!
        descriptorWrites.reserve(128);
        bufferInfos.reserve(128);
        imageInfos.reserve(128);
        texelBufferViews.reserve(128);
        accelerationStructureViews.reserve(128);

        VkResult res;

        // Create descriptor pool:
        VkDescriptorPoolSize poolSizes[9] = {};

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = GPU_RESOURCE_HEAP_CBV_COUNT * poolSize;

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[1].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        poolSizes[2].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;

        poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[3].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;

        poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[4].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;

        poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        poolSizes[5].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;

        poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[6].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;

        poolSizes[7].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSizes[7].descriptorCount = GPU_SAMPLER_HEAP_COUNT * poolSize;

        poolSizes[8].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        poolSizes[8].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = ALIMER_STATIC_ARRAY_SIZE(poolSizes);
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = poolSize;
        //poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        res = vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool);
        assert(res == VK_SUCCESS);

        reset();
    }

    void DescriptorTableFrameAllocator::destroy()
    {
        if (descriptorPool != VK_NULL_HANDLE)
        {
            device->allocationhandler->destroyer_descriptorPools.push_back(std::make_pair(descriptorPool, device->FRAMECOUNT));
            descriptorPool = VK_NULL_HANDLE;
        }
    }

    void DescriptorTableFrameAllocator::reset()
    {
        dirty = true;

        if (descriptorPool != VK_NULL_HANDLE)
        {
            VkResult res = vkResetDescriptorPool(device->device, descriptorPool, 0);
            assert(res == VK_SUCCESS);
        }

        memset(CBV, 0, sizeof(CBV));
        memset(SRV, 0, sizeof(SRV));
        memset(SRV_index, -1, sizeof(SRV_index));
        memset(UAV, 0, sizeof(UAV));
        memset(UAV_index, -1, sizeof(UAV_index));
        memset(SAM, 0, sizeof(SAM));
    }

    void DescriptorTableFrameAllocator::validate(bool graphics, Vulkan_CommandList* commandList, bool raytracing)
    {
        if (!dirty)
            return;
        dirty = true;

        auto pso_internal = graphics ? to_internal(commandList->active_pso) : nullptr;
        auto cs_internal = graphics ? nullptr : to_internal(commandList->active_cs);

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        if (graphics)
        {
            pipelineLayout = pso_internal->pipelineLayout;
            descriptorSetLayout = pso_internal->descriptorSetLayout;
        }
        else
        {
            pipelineLayout = cs_internal->pipelineLayout_cs;
            descriptorSetLayout = cs_internal->descriptorSetLayout;
        }

        VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkResult res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
        while (res == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            poolSize *= 2;
            destroy();
            init(device);
            allocInfo.descriptorPool = descriptorPool;
            res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
        }
        assert(res == VK_SUCCESS);

        descriptorWrites.clear();
        bufferInfos.clear();
        imageInfos.clear();
        texelBufferViews.clear();
        accelerationStructureViews.clear();

        const auto& layoutBindings = graphics ? pso_internal->layoutBindings : cs_internal->layoutBindings;
        const auto& imageViewTypes = graphics ? pso_internal->imageViewTypes : cs_internal->imageViewTypes;

        int i = 0;
        for (auto& x : layoutBindings)
        {
            descriptorWrites.emplace_back();
            auto& write = descriptorWrites.back();
            write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstArrayElement = 0;
            write.descriptorType = x.descriptorType;
            write.dstBinding = x.binding;
            write.descriptorCount = 1;

            VkImageViewType viewtype = imageViewTypes[i++];

            switch (x.descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                {
                    imageInfos.emplace_back();
                    write.pImageInfo = &imageInfos.back();
                    imageInfos.back() = {};

                    const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_S;
                    const Sampler* sampler = SAM[original_binding];
                    if (sampler == nullptr)
                    {
                        imageInfos.back().sampler = device->nullSampler;
                    }
                    else
                    {
                        imageInfos.back().sampler = to_internal(sampler)->resource;
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                {
                    imageInfos.emplace_back();
                    write.pImageInfo = &imageInfos.back();
                    imageInfos.back() = {};
                    imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_GENERAL;

                    const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_T;
                    const GPUResource* resource = SRV[original_binding];
                    if (resource == nullptr || !resource->IsValid() || !resource->IsTexture())
                    {
                        switch (viewtype)
                        {
                            case VK_IMAGE_VIEW_TYPE_1D:
                                imageInfos.back().imageView = device->nullImageView1D;
                                break;
                            case VK_IMAGE_VIEW_TYPE_2D:
                                imageInfos.back().imageView = device->nullImageView2D;
                                break;
                            case VK_IMAGE_VIEW_TYPE_3D:
                                imageInfos.back().imageView = device->nullImageView3D;
                                break;
                            case VK_IMAGE_VIEW_TYPE_CUBE:
                                imageInfos.back().imageView = device->nullImageViewCube;
                                break;
                            case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
                                imageInfos.back().imageView = device->nullImageView1DArray;
                                break;
                            case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
                                imageInfos.back().imageView = device->nullImageView2DArray;
                                break;
                            case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
                                imageInfos.back().imageView = device->nullImageViewCubeArray;
                                break;
                            case VK_IMAGE_VIEW_TYPE_MAX_ENUM:
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        int subresource = SRV_index[original_binding];
                        const Texture* texture = (const Texture*) resource;
                        if (subresource >= 0)
                        {
                            imageInfos.back().imageView = to_internal(texture)->subresources_srv[subresource];
                        }
                        else
                        {
                            imageInfos.back().imageView = to_internal(texture)->srv;
                        }

                        VkImageLayout layout = _ConvertImageLayout(texture->desc.layout);
                        if (layout != VK_IMAGE_LAYOUT_GENERAL && layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                        {
                            // Means texture initial layout is not compatible, so it must have been transitioned
                            layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        }
                        imageInfos.back().imageLayout = layout;
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    imageInfos.emplace_back();
                    write.pImageInfo = &imageInfos.back();
                    imageInfos.back() = {};
                    imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_GENERAL;

                    const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_U;
                    const GPUResource* resource = UAV[original_binding];
                    if (resource == nullptr || !resource->IsValid() || !resource->IsTexture())
                    {
                        switch (viewtype)
                        {
                            case VK_IMAGE_VIEW_TYPE_1D:
                                imageInfos.back().imageView = device->nullImageView1D;
                                break;
                            case VK_IMAGE_VIEW_TYPE_2D:
                                imageInfos.back().imageView = device->nullImageView2D;
                                break;
                            case VK_IMAGE_VIEW_TYPE_3D:
                                imageInfos.back().imageView = device->nullImageView3D;
                                break;
                            case VK_IMAGE_VIEW_TYPE_CUBE:
                                imageInfos.back().imageView = device->nullImageViewCube;
                                break;
                            case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
                                imageInfos.back().imageView = device->nullImageView1DArray;
                                break;
                            case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
                                imageInfos.back().imageView = device->nullImageView2DArray;
                                break;
                            case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
                                imageInfos.back().imageView = device->nullImageViewCubeArray;
                                break;
                            case VK_IMAGE_VIEW_TYPE_MAX_ENUM:
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        int subresource = UAV_index[original_binding];
                        const Texture* texture = (const Texture*) resource;
                        if (subresource >= 0)
                        {
                            imageInfos.back().imageView = to_internal(texture)->subresources_uav[subresource];
                        }
                        else
                        {
                            imageInfos.back().imageView = to_internal(texture)->uav;
                        }
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                {
                    bufferInfos.emplace_back();
                    write.pBufferInfo = &bufferInfos.back();
                    bufferInfos.back() = {};

                    const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_B;
                    const GraphicsBuffer* buffer = CBV[original_binding];
                    if (buffer == nullptr)
                    {
                        bufferInfos.back().buffer = device->nullBuffer;
                        bufferInfos.back().range = VK_WHOLE_SIZE;
                    }
                    else
                    {
                        auto internal_state = to_internal(buffer);
                        if (buffer->GetDesc().Usage == USAGE_DYNAMIC)
                        {
                            const GPUAllocation& allocation = internal_state->dynamic[commandList->index];
                            bufferInfos.back().buffer = to_internal(allocation.buffer)->resource;
                            bufferInfos.back().offset = allocation.offset;
                            bufferInfos.back().range = buffer->GetDesc().ByteWidth;
                        }
                        else
                        {
                            bufferInfos.back().buffer = internal_state->resource;
                            bufferInfos.back().offset = 0;
                            bufferInfos.back().range = buffer->GetDesc().ByteWidth;
                        }
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                {
                    texelBufferViews.emplace_back();
                    write.pTexelBufferView = &texelBufferViews.back();
                    texelBufferViews.back() = {};

                    const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_T;
                    const GPUResource* resource = SRV[original_binding];
                    if (resource == nullptr || !resource->IsValid() || !resource->IsBuffer())
                    {
                        texelBufferViews.back() = device->nullBufferView;
                    }
                    else
                    {
                        int subresource = SRV_index[original_binding];
                        const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
                        if (subresource >= 0)
                        {
                            texelBufferViews.back() = to_internal(buffer)->subresources_srv[subresource];
                        }
                        else
                        {
                            texelBufferViews.back() = to_internal(buffer)->srv;
                        }
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                {
                    texelBufferViews.emplace_back();
                    write.pTexelBufferView = &texelBufferViews.back();
                    texelBufferViews.back() = {};

                    const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_U;
                    const GPUResource* resource = UAV[original_binding];
                    if (resource == nullptr || !resource->IsValid() || !resource->IsBuffer())
                    {
                        texelBufferViews.back() = device->nullBufferView;
                    }
                    else
                    {
                        int subresource = UAV_index[original_binding];
                        const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
                        if (subresource >= 0)
                        {
                            texelBufferViews.back() = to_internal(buffer)->subresources_uav[subresource];
                        }
                        else
                        {
                            texelBufferViews.back() = to_internal(buffer)->uav;
                        }
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                {
                    bufferInfos.emplace_back();
                    write.pBufferInfo = &bufferInfos.back();
                    bufferInfos.back() = {};

                    if (x.binding < VULKAN_BINDING_SHIFT_U)
                    {
                        // SRV
                        const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_T;
                        const GPUResource* resource = SRV[original_binding];
                        if (resource == nullptr || !resource->IsValid() || !resource->IsBuffer())
                        {
                            bufferInfos.back().buffer = device->nullBuffer;
                            bufferInfos.back().range = VK_WHOLE_SIZE;
                        }
                        else
                        {
                            int subresource = SRV_index[original_binding];
                            const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
                            bufferInfos.back().buffer = to_internal(buffer)->resource;
                            bufferInfos.back().range = buffer->GetDesc().ByteWidth;
                        }
                    }
                    else
                    {
                        // UAV
                        const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_U;
                        const GPUResource* resource = UAV[original_binding];
                        if (resource == nullptr || !resource->IsValid() || !resource->IsBuffer())
                        {
                            bufferInfos.back().buffer = device->nullBuffer;
                            bufferInfos.back().range = VK_WHOLE_SIZE;
                        }
                        else
                        {
                            int subresource = UAV_index[original_binding];
                            const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
                            bufferInfos.back().buffer = to_internal(buffer)->resource;
                            bufferInfos.back().range = buffer->GetDesc().ByteWidth;
                        }
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                {
                    accelerationStructureViews.emplace_back();
                    write.pNext = &accelerationStructureViews.back();
                    accelerationStructureViews.back() = {};
                    accelerationStructureViews.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                    accelerationStructureViews.back().accelerationStructureCount = 1;

                    const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_T;
                    const GPUResource* resource = SRV[original_binding];
                    if (resource == nullptr || !resource->IsValid() || !resource->IsAccelerationStructure())
                    {
                        assert(0); // invalid acceleration structure!
                    }
                    else
                    {
                        const RaytracingAccelerationStructure* as = (const RaytracingAccelerationStructure*) resource;
                        accelerationStructureViews.back().pAccelerationStructures = &to_internal(as)->resource;
                    }
                }
                break;
            }
        }

        vkUpdateDescriptorSets(device->device, (uint32_t) descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        vkCmdBindDescriptorSets(commandList->GetDirectCommandList(),
                                graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : (raytracing ? VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR : VK_PIPELINE_BIND_POINT_COMPUTE),
                                pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    }

    VkDescriptorSet DescriptorTableFrameAllocator::commit(const DescriptorTable* table)
    {
        auto internal_state = to_internal(table);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &internal_state->layout;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkResult res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
        while (res == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            poolSize *= 2;
            destroy();
            init(device);
            allocInfo.descriptorPool = descriptorPool;
            res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
        }
        assert(res == VK_SUCCESS);

        vkUpdateDescriptorSetWithTemplate(
            device->device,
            descriptorSet,
            internal_state->updatetemplate,
            internal_state->descriptors.data());

        return descriptorSet;
    }

    bool GraphicsDevice_Vulkan::IsAvailable()
    {
        static bool available = false;
        static bool available_initialized = false;

        if (available_initialized)
        {
            return available;
        }

        available_initialized = true;

        VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
        {
            return false;
        }

        VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
        applicationInfo.apiVersion = volkGetInstanceVersion();

        VkInstanceCreateInfo instanceInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.pApplicationInfo = &applicationInfo;

        VkInstance tempInstance;
        if (vkCreateInstance(&instanceInfo, nullptr, &tempInstance) != VK_SUCCESS)
        {
            return false;
        }

        volkLoadInstanceOnly(tempInstance);

        vkDestroyInstance(tempInstance, nullptr);
        available = true;
        return true;
    }

    GraphicsDevice_Vulkan::GraphicsDevice_Vulkan(WindowHandle window, const GraphicsSettings& settings)
        : Graphics(window, settings)
    {
        if (!IsAvailable())
        {
            // TODO: MessageBox
        }

        DESCRIPTOR_MANAGEMENT = true;
        TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(VkAccelerationStructureInstanceKHR);

        const bool enableDebugLayer = any(settings.flags & GraphicsDeviceFlags::DebugRuntime) || any(settings.flags & GraphicsDeviceFlags::GPUBasedValidation);

        VkResult res;

        // Create instance first
        {
            VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
            appInfo.pApplicationName = settings.applicationName.c_str();
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Alimer Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(ALIMER_VERSION_MAJOR, ALIMER_VERSION_MINOR, ALIMER_VERSION_PATCH);
            appInfo.apiVersion = VK_API_VERSION_1_2;

            // Enumerate available extensions:
            uint32_t availableInstanceExtensionCount = 0;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, nullptr));
            std::vector<VkExtensionProperties> availableInstanceExtensions(availableInstanceExtensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, availableInstanceExtensions.data()));

            for (auto& availableExtension : availableInstanceExtensions)
            {
                if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
                {
                    debugUtils = true;
                }
            }

            std::vector<const char*> enabledExtensions;
            if (debugUtils)
            {
                enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef VK_USE_PLATFORM_WIN32_KHR
            enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif SDL2
            {
                uint32_t extensionCount;
                SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
                std::vector<const char*> extensionNames_sdl(extensionCount);
                SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames_sdl.data());
                enabledExtensions.reserve(enabledExtensions.size() + extensionNames_sdl.size());
                enabledExtensions.insert(enabledExtensions.begin(), extensionNames_sdl.cbegin(), extensionNames_sdl.cend());
            }
#endif // _WIN32

            bool enableValidationLayers = enableDebugLayer;

            if (enableDebugLayer && !checkValidationLayerSupport())
            {
                //wiHelper::messageBox("Vulkan validation layer requested but not available!");
                enableValidationLayers = false;
            }

            // Create instance:
            {
                VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
                instanceCreateInfo.pApplicationInfo = &appInfo;
                instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
                instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
                instanceCreateInfo.enabledLayerCount = 0;
                if (enableValidationLayers)
                {
                    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
                }

                VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
                if (debugUtils)
                {
                    debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                    debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                    debugUtilsCreateInfo.pfnUserCallback = debugUtilsMessengerCallback;

                    instanceCreateInfo.pNext = &debugUtilsCreateInfo;
                }

                VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

                volkLoadInstance(instance);

                if (enableDebugLayer && debugUtils)
                {
                    VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
                    if (result != VK_SUCCESS)
                    {
                        VK_LOG_ERROR(result, "Could not create debug utils messenger");
                    }
                }
            }
        }

        // Surface creation:
        {
#ifdef VK_USE_PLATFORM_WIN32_KHR
            VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            createInfo.hwnd = (HWND) window;
            createInfo.hinstance = GetModuleHandleW(nullptr);

            if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
            {
                assert(0);
            }
#elif SDL2
            if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
            {
                throw sdl2::SDLError("Error creating a vulkan surface");
            }
#else
#    error WICKEDENGINE VULKAN DEVICE ERROR: PLATFORM NOT SUPPORTED
#endif // _WIN32
        }

        // Enumerating and creating devices:
        {
            uint32_t deviceCount = 0;
            VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
            assert(res == VK_SUCCESS);

            if (deviceCount == 0)
            {
                //wiHelper::messageBox("failed to find GPUs with Vulkan support!");
                assert(0);
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            res = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
            assert(res == VK_SUCCESS);

            device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            device_properties_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
            device_properties_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
            raytracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
            mesh_shader_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;

            device_properties.pNext = &device_properties_1_1;
            device_properties_1_1.pNext = &device_properties_1_2;
            device_properties_1_2.pNext = &raytracing_properties;
            raytracing_properties.pNext = &mesh_shader_properties;

            for (const auto& device : devices)
            {
                if (isDeviceSuitable(device, surface))
                {
                    vkGetPhysicalDeviceProperties2(device, &device_properties);
                    bool discrete = device_properties.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                    if (discrete || physicalDevice == VK_NULL_HANDLE)
                    {
                        physicalDevice = device;
                        if (discrete)
                        {
                            break; // if this is discrete GPU, look no further (prioritize discrete GPU)
                        }
                    }
                }
            }

            if (physicalDevice == VK_NULL_HANDLE)
            {
                //wiHelper::messageBox("failed to find a suitable GPU!");
                assert(0);
            }

            queueIndices = findQueueFamilies(physicalDevice, surface);

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<int> uniqueQueueFamilies = {queueIndices.graphicsFamily, queueIndices.presentFamily, queueIndices.copyFamily};

            float queuePriority = 1.0f;
            for (int queueFamily : uniqueQueueFamilies)
            {
                VkDeviceQueueCreateInfo queueCreateInfo = {};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            assert(device_properties.properties.limits.timestampComputeAndGraphics == VK_TRUE);

            uint32_t deviceExtensionCount;
            res = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
            assert(res == VK_SUCCESS);
            std::vector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
            res = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtensions.data());
            assert(res == VK_SUCCESS);

            std::vector<const char*> enabled_deviceExtensions = required_deviceExtensions;
            if (checkDeviceExtensionSupport(VK_KHR_SPIRV_1_4_EXTENSION_NAME, availableDeviceExtensions))
            {
                enabled_deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
            }

            device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

            device_features2.pNext = &features_1_1;
            features_1_1.pNext = &features_1_2;

#ifdef ENABLE_RAYTRACING_EXTENSION
            raytracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
            if (checkDeviceExtensionSupport(VK_KHR_RAY_TRACING_EXTENSION_NAME, availableDeviceExtensions))
            {
                SHADER_IDENTIFIER_SIZE = raytracing_properties.shaderGroupHandleSize;
                RAYTRACING = true;
                enabled_deviceExtensions.push_back(VK_KHR_RAY_TRACING_EXTENSION_NAME);
                enabled_deviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
                enabled_deviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
                enabled_deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                features_1_2.pNext = &raytracing_features;
            }
#endif // ENABLE_RAYTRACING_EXTENSION

            mesh_shader_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
            if (checkDeviceExtensionSupport(VK_NV_MESH_SHADER_EXTENSION_NAME, availableDeviceExtensions))
            {
                enabled_deviceExtensions.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);
                if (RAYTRACING)
                {
                    raytracing_features.pNext = &mesh_shader_features;
                }
                else
                {
                    features_1_2.pNext = &mesh_shader_features;
                }
            }

            vkGetPhysicalDeviceFeatures2(physicalDevice, &device_features2);

            assert(device_features2.features.imageCubeArray == VK_TRUE);
            assert(device_features2.features.independentBlend == VK_TRUE);
            assert(device_features2.features.geometryShader == VK_TRUE);
            assert(device_features2.features.samplerAnisotropy == VK_TRUE);
            assert(device_features2.features.shaderClipDistance == VK_TRUE);
            assert(device_features2.features.textureCompressionBC == VK_TRUE);
            assert(device_features2.features.occlusionQueryPrecise == VK_TRUE);
            TESSELLATION = device_features2.features.tessellationShader == VK_TRUE;
            UAV_LOAD_FORMAT_COMMON = device_features2.features.shaderStorageImageExtendedFormats == VK_TRUE;
            RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = true; // let's hope for the best...

            if (RAYTRACING)
            {
                assert(features_1_2.bufferDeviceAddress == VK_TRUE);
            }

            if (mesh_shader_features.meshShader == VK_TRUE && mesh_shader_features.taskShader == VK_TRUE)
            {
                // Enable mesh shader here (problematic with certain driver versions, disabled by default):
                //MESH_SHADER = true;
            }

            VkFormatProperties formatProperties = {0};
            vkGetPhysicalDeviceFormatProperties(physicalDevice, _ConvertFormat(PixelFormat::RG11B10Float), &formatProperties);
            UAV_LOAD_FORMAT_R11G11B10_FLOAT = formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

            VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();

            createInfo.pEnabledFeatures = nullptr;
            createInfo.pNext = &device_features2;

            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabled_deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = enabled_deviceExtensions.data();

            res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
            assert(res == VK_SUCCESS);

            //volkLoadDeviceTable(&device_table, device);

            vkGetDeviceQueue(device, queueIndices.graphicsFamily, 0, &graphicsQueue);
            vkGetDeviceQueue(device, queueIndices.presentFamily, 0, &presentQueue);
        }

        allocationhandler = std::make_shared<AllocationHandler>();
        allocationhandler->device = device;
        allocationhandler->instance = instance;

        // Initialize Vulkan Memory Allocator helper:
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        if (features_1_2.bufferDeviceAddress)
        {
            allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }
        res = vmaCreateAllocator(&allocatorInfo, &allocationhandler->allocator);
        assert(res == VK_SUCCESS);

        if (RAYTRACING)
        {
            createRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR) vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
            createAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR) vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
            bindAccelerationStructureMemoryKHR = (PFN_vkBindAccelerationStructureMemoryKHR) vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryKHR");
            destroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR) vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
            getAccelerationStructureMemoryRequirementsKHR = (PFN_vkGetAccelerationStructureMemoryRequirementsKHR) vkGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsKHR");
            getAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR) vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
            getRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR) vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
            cmdBuildAccelerationStructureKHR = (PFN_vkCmdBuildAccelerationStructureKHR) vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureKHR");
            cmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR) vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
        }

        if (MESH_SHADER)
        {
            cmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV) vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksNV");
            cmdDrawMeshTasksIndirectNV = (PFN_vkCmdDrawMeshTasksIndirectNV) vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectNV");
        }

        CreateBackBufferResources();

        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

        // Create frame resources:
        {
            for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
            {
                // Fence:
                {
                    VkFenceCreateInfo fenceInfo = {};
                    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    //fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                    VkResult res = vkCreateFence(device, &fenceInfo, nullptr, &frames[fr].frameFence);
                    assert(res == VK_SUCCESS);
                }

                // Create resources for transition command buffer:
                {
                    VkCommandPoolCreateInfo poolInfo = {};
                    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
                    poolInfo.flags = 0; // Optional

                    res = vkCreateCommandPool(device, &poolInfo, nullptr, &frames[fr].transitionCommandPool);
                    assert(res == VK_SUCCESS);

                    VkCommandBufferAllocateInfo commandBufferInfo = {};
                    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    commandBufferInfo.commandBufferCount = 1;
                    commandBufferInfo.commandPool = frames[fr].transitionCommandPool;
                    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

                    res = vkAllocateCommandBuffers(device, &commandBufferInfo, &frames[fr].transitionCommandBuffer);
                    assert(res == VK_SUCCESS);

                    VkCommandBufferBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    beginInfo.pInheritanceInfo = nullptr; // Optional

                    res = vkBeginCommandBuffer(frames[fr].transitionCommandBuffer, &beginInfo);
                    assert(res == VK_SUCCESS);
                }

                // Create resources for copy (transfer) queue:
                {
                    vkGetDeviceQueue(device, queueIndices.copyFamily, 0, &frames[fr].copyQueue);

                    VkCommandPoolCreateInfo poolInfo = {};
                    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    poolInfo.queueFamilyIndex = queueFamilyIndices.copyFamily;
                    poolInfo.flags = 0; // Optional

                    res = vkCreateCommandPool(device, &poolInfo, nullptr, &frames[fr].copyCommandPool);
                    assert(res == VK_SUCCESS);

                    VkCommandBufferAllocateInfo commandBufferInfo = {};
                    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    commandBufferInfo.commandBufferCount = 1;
                    commandBufferInfo.commandPool = frames[fr].copyCommandPool;
                    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

                    res = vkAllocateCommandBuffers(device, &commandBufferInfo, &frames[fr].copyCommandBuffer);
                    assert(res == VK_SUCCESS);

                    VkCommandBufferBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    beginInfo.pInheritanceInfo = nullptr; // Optional

                    res = vkBeginCommandBuffer(frames[fr].copyCommandBuffer, &beginInfo);
                    assert(res == VK_SUCCESS);
                }
            }
        }

        // Create copy semaphore
        {
            VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &copySemaphore);
            assert(res == VK_SUCCESS);
        }

        // Create default null descriptors:
        {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = 4;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bufferInfo.flags = 0;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &nullBuffer, &nullBufferAllocation, nullptr);
            assert(res == VK_SUCCESS);

            VkBufferViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            viewInfo.range = VK_WHOLE_SIZE;
            viewInfo.buffer = nullBuffer;
            res = vkCreateBufferView(device, &viewInfo, nullptr, &nullBufferView);
            assert(res == VK_SUCCESS);
        }
        {
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.extent.width = 1;
            imageInfo.extent.height = 1;
            imageInfo.extent.depth = 1;
            imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageInfo.arrayLayers = 1;
            imageInfo.mipLevels = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            imageInfo.flags = 0;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            imageInfo.imageType = VK_IMAGE_TYPE_1D;
            res = vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage1D, &nullImageAllocation1D, nullptr);
            assert(res == VK_SUCCESS);

            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            imageInfo.arrayLayers = 6;
            res = vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage2D, &nullImageAllocation2D, nullptr);
            assert(res == VK_SUCCESS);

            imageInfo.imageType = VK_IMAGE_TYPE_3D;
            imageInfo.flags = 0;
            imageInfo.arrayLayers = 1;
            res = vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage3D, &nullImageAllocation3D, nullptr);
            assert(res == VK_SUCCESS);

            // Transitions:
            copyQueueLock.lock();
            {
                auto& frame = GetFrameResources();
                if (!copyQueueUse)
                {
                    copyQueueUse = true;

                    res = vkResetCommandPool(device, frame.copyCommandPool, 0);
                    assert(res == VK_SUCCESS);

                    VkCommandBufferBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                    beginInfo.pInheritanceInfo = nullptr; // Optional

                    res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
                    assert(res == VK_SUCCESS);
                }

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = imageInfo.initialLayout;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = nullImage1D;
                barrier.subresourceRange.layerCount = 1;
                frame.loadedimagetransitions.push_back(barrier);
                barrier.image = nullImage2D;
                barrier.subresourceRange.layerCount = 6;
                frame.loadedimagetransitions.push_back(barrier);
                barrier.image = nullImage3D;
                barrier.subresourceRange.layerCount = 1;
                frame.loadedimagetransitions.push_back(barrier);
            }
            copyQueueLock.unlock();

            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

            viewInfo.image = nullImage1D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
            res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView1D);
            assert(res == VK_SUCCESS);

            viewInfo.image = nullImage1D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView1DArray);
            assert(res == VK_SUCCESS);

            viewInfo.image = nullImage2D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView2D);
            assert(res == VK_SUCCESS);

            viewInfo.image = nullImage2D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView2DArray);
            assert(res == VK_SUCCESS);

            viewInfo.image = nullImage2D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            viewInfo.subresourceRange.layerCount = 6;
            res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageViewCube);
            assert(res == VK_SUCCESS);

            viewInfo.image = nullImage2D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            viewInfo.subresourceRange.layerCount = 6;
            res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageViewCubeArray);
            assert(res == VK_SUCCESS);

            viewInfo.image = nullImage3D;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView3D);
            assert(res == VK_SUCCESS);
        }
        {
            VkSamplerCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

            res = vkCreateSampler(device, &createInfo, nullptr, &nullSampler);
            assert(res == VK_SUCCESS);
        }

        // GPU Queries:
        {
            timestamp_frequency = uint64_t(1.0 / double(device_properties.properties.limits.timestampPeriod) * 1000 * 1000 * 1000);

            VkQueryPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;

            for (uint32_t i = 0; i < timestamp_query_count; ++i)
            {
                allocationhandler->free_timestampqueries.push_back(i);
            }
            poolInfo.queryCount = timestamp_query_count;
            poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
            res = vkCreateQueryPool(device, &poolInfo, nullptr, &querypool_timestamp);
            assert(res == VK_SUCCESS);
            timestamps_to_reset.reserve(timestamp_query_count);

            for (uint32_t i = 0; i < occlusion_query_count; ++i)
            {
                allocationhandler->free_occlusionqueries.push_back(i);
            }
            poolInfo.queryCount = occlusion_query_count;
            poolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
            res = vkCreateQueryPool(device, &poolInfo, nullptr, &querypool_occlusion);
            assert(res == VK_SUCCESS);
            occlusions_to_reset.reserve(occlusion_query_count);
        }

        LOGI("Vulkan Graphics Device created");
    }

    GraphicsDevice_Vulkan::~GraphicsDevice_Vulkan()
    {
        VK_CHECK(vkDeviceWaitIdle(device));

        for (uint32_t i = 0; i < kCommandListCount; i++)
        {
            if (!commandLists[i])
                break;

            delete commandLists[i];
        }

        for (auto& frame : frames)
        {
            vkDestroyFence(device, frame.frameFence, nullptr);
            vkDestroyCommandPool(device, frame.transitionCommandPool, nullptr);
            vkDestroyCommandPool(device, frame.copyCommandPool, nullptr);
        }

        for (auto semaphore : recycledSemaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }

        vkDestroySemaphore(device, copySemaphore, nullptr);

        for (auto& x : pipelines_global)
        {
            vkDestroyPipeline(device, x.second, nullptr);
        }

        vkDestroyQueryPool(device, querypool_timestamp, nullptr);
        vkDestroyQueryPool(device, querypool_occlusion, nullptr);

        vmaDestroyBuffer(allocationhandler->allocator, nullBuffer, nullBufferAllocation);
        vkDestroyBufferView(device, nullBufferView, nullptr);
        vmaDestroyImage(allocationhandler->allocator, nullImage1D, nullImageAllocation1D);
        vmaDestroyImage(allocationhandler->allocator, nullImage2D, nullImageAllocation2D);
        vmaDestroyImage(allocationhandler->allocator, nullImage3D, nullImageAllocation3D);
        vkDestroyImageView(device, nullImageView1D, nullptr);
        vkDestroyImageView(device, nullImageView1DArray, nullptr);
        vkDestroyImageView(device, nullImageView2D, nullptr);
        vkDestroyImageView(device, nullImageView2DArray, nullptr);
        vkDestroyImageView(device, nullImageViewCube, nullptr);
        vkDestroyImageView(device, nullImageViewCubeArray, nullptr);
        vkDestroyImageView(device, nullImageView3D, nullptr);
        vkDestroySampler(device, nullSampler, nullptr);

        vkDestroyRenderPass(device, defaultRenderPass, nullptr);
        for (size_t i = 0; i < swapChainImages.size(); ++i)
        {
            vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
            vkDestroyImageView(device, swapChainImageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);

        allocationhandler->Update(~0, 0); // destroy all remaining
        if (allocationhandler->allocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(allocationhandler->allocator, &stats);

            if (stats.total.usedBytes > 0)
            {
                LOGE("Total device memory leaked: {} bytes.", stats.total.usedBytes);
            }

            vmaDestroyAllocator(allocationhandler->allocator);
            allocationhandler->allocator = VK_NULL_HANDLE;
        }

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);

        if (debugUtilsMessenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }

    void GraphicsDevice_Vulkan::CreateBackBufferResources()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

        VkSurfaceFormatKHR surfaceFormat = {};
        surfaceFormat.format = _ConvertFormat(BACKBUFFER_FORMAT);
        bool valid = false;

        for (const auto& format : swapChainSupport.formats)
        {
            if (format.format == surfaceFormat.format)
            {
                surfaceFormat = format;
                valid = true;
                break;
            }
        }
        if (!valid)
        {
            surfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
            BACKBUFFER_FORMAT = PixelFormat::BGRA8Unorm;
        }

        swapChainExtent = {backbufferWidth, backbufferHeight};
        swapChainExtent.width = std::max(swapChainSupport.capabilities.minImageExtent.width, std::min(swapChainSupport.capabilities.maxImageExtent.width, swapChainExtent.width));
        swapChainExtent.height = std::max(swapChainSupport.capabilities.minImageExtent.height, std::min(swapChainSupport.capabilities.maxImageExtent.height, swapChainExtent.height));

        uint32_t imageCount = BACKBUFFER_COUNT;

        VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = swapChainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        uint32_t queueFamilyIndices[] = {(uint32_t) queueIndices.graphicsFamily, (uint32_t) queueIndices.presentFamily};

        if (queueIndices.graphicsFamily != queueIndices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // The only one that is always supported
        if (!verticalSync)
        {
            // The immediate present mode is not necessarily supported:
            for (auto& presentmode : swapChainSupport.presentModes)
            {
                if (presentmode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                    break;
                }
            }
        }
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = swapChain;

        VkResult res = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
        assert(res == VK_SUCCESS);

        if (createInfo.oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, createInfo.oldSwapchain, nullptr);
        }

        res = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        assert(res == VK_SUCCESS);
        assert(BACKBUFFER_COUNT <= imageCount);
        swapChainImages.resize(imageCount);
        res = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        assert(res == VK_SUCCESS);
        swapChainImageFormat = surfaceFormat.format;

        if (debugUtils)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
            nameInfo.pObjectName = "SWAPCHAIN";
            nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
            for (auto& x : swapChainImages)
            {
                nameInfo.objectHandle = (uint64_t) x;

                vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
            }
        }

        // Create default render pass:
        {
            VkAttachmentDescription colorAttachment = {};
            colorAttachment.format = swapChainImageFormat;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef = {};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;

            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (defaultRenderPass != VK_NULL_HANDLE)
            {
                allocationhandler->destroyer_renderpasses.push_back(std::make_pair(defaultRenderPass, allocationhandler->framecount));
            }
            res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &defaultRenderPass);
            assert(res == VK_SUCCESS);
        }

        // Create swap chain render targets:
        swapChainImageViews.resize(swapChainImages.size());
        swapChainFramebuffers.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); ++i)
        {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (swapChainImageViews[i] != VK_NULL_HANDLE)
            {
                allocationhandler->destroyer_imageviews.push_back(std::make_pair(swapChainImageViews[i], allocationhandler->framecount));
            }
            res = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]);
            assert(res == VK_SUCCESS);

            VkImageView attachments[] = {
                swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = defaultRenderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (swapChainFramebuffers[i] != VK_NULL_HANDLE)
            {
                allocationhandler->destroyer_framebuffers.push_back(std::make_pair(swapChainFramebuffers[i], allocationhandler->framecount));
            }
            res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
            assert(res == VK_SUCCESS);
        }

        backbufferWidth = swapChainExtent.width;
        backbufferHeight = swapChainExtent.height;
    }

    void GraphicsDevice_Vulkan::Resize(uint32_t width, uint32_t height)
    {
        if (width != backbufferWidth || height != backbufferHeight)
        {
            backbufferWidth = width;
            backbufferHeight = height;

            CreateBackBufferResources();
        }
    }

    Texture GraphicsDevice_Vulkan::GetBackBuffer()
    {
        auto internal_state = std::make_shared<Texture_Vulkan>();
        internal_state->resource = swapChainImages[swapChainImageIndex];

        Texture result;
        result.type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;
        result.internal_state = internal_state;
        result.desc.type = TextureDesc::TEXTURE_2D;
        result.desc.Width = swapChainExtent.width;
        result.desc.Height = swapChainExtent.height;
        result.desc.format = BACKBUFFER_FORMAT;
        return result;
    }

    RefPtr<GraphicsBuffer> GraphicsDevice_Vulkan::CreateBuffer(const GPUBufferDesc& desc, const void* initialData)
    {
        RefPtr<Buffer_Vulkan> result(new Buffer_Vulkan(desc));
        result->allocationhandler = allocationhandler;

        if (desc.Usage == USAGE_DYNAMIC && desc.BindFlags & BIND_CONSTANT_BUFFER)
        {
            // this special case will use frame allocator
            return result;
        }

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = desc.ByteWidth;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (desc.BindFlags & BIND_VERTEX_BUFFER)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (desc.BindFlags & BIND_INDEX_BUFFER)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (desc.BindFlags & BIND_CONSTANT_BUFFER)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if (desc.BindFlags & BIND_SHADER_RESOURCE)
        {
            if (desc.format == PixelFormat::Invalid)
            {
                bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            else
            {
                bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            }
        }
        if (desc.BindFlags & BIND_UNORDERED_ACCESS)
        {
            if (desc.format == PixelFormat::Invalid)
            {
                bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            else
            {
                bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
            }
        }
        if (desc.MiscFlags & RESOURCE_MISC_INDIRECT_ARGS)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }
        if (desc.MiscFlags & RESOURCE_MISC_RAY_TRACING)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR;
        }
        if (features_1_2.bufferDeviceAddress == VK_TRUE)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }
        bufferInfo.flags = 0;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkResult res;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        if (desc.Usage == USAGE_STAGING)
        {
            if (desc.CPUAccessFlags & CPU_ACCESS_READ)
            {
                allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
                bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            }
            else
            {
                allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
                allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            }
        }

        res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &result->resource, &result->allocation, nullptr);
        assert(res == VK_SUCCESS);

        // Issue data copy on request:
        if (initialData != nullptr)
        {
            GPUBufferDesc uploadBufferDesc = {};
            uploadBufferDesc.ByteWidth = desc.ByteWidth;
            uploadBufferDesc.Usage = USAGE_STAGING;

            RefPtr<GraphicsBuffer> uploadbuffer = CreateBuffer(uploadBufferDesc, nullptr);
            ALIMER_ASSERT(uploadbuffer.IsNotNull());
            VkBuffer upload_resource = to_internal(uploadbuffer)->resource;
            VmaAllocation upload_allocation = to_internal(uploadbuffer)->allocation;

            void* pData = upload_allocation->GetMappedData();
            ALIMER_ASSERT(pData != nullptr);

            memcpy(pData, initialData, desc.ByteWidth);

            copyQueueLock.lock();
            {
                auto& frame = GetFrameResources();
                if (!copyQueueUse)
                {
                    copyQueueUse = true;

                    res = vkResetCommandPool(device, frame.copyCommandPool, 0);
                    assert(res == VK_SUCCESS);

                    VkCommandBufferBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                    beginInfo.pInheritanceInfo = nullptr; // Optional

                    res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
                    assert(res == VK_SUCCESS);
                }

                VkBufferCopy copyRegion = {};
                copyRegion.size = desc.ByteWidth;
                copyRegion.srcOffset = 0;
                copyRegion.dstOffset = 0;

                VkBufferMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.buffer = result->resource;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.size = VK_WHOLE_SIZE;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                vkCmdPipelineBarrier(
                    frame.copyCommandBuffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    1, &barrier,
                    0, nullptr);

                vkCmdCopyBuffer(frame.copyCommandBuffer, upload_resource, result->resource, 1, &copyRegion);

                VkAccessFlags tmp = barrier.srcAccessMask;
                barrier.srcAccessMask = barrier.dstAccessMask;
                barrier.dstAccessMask = 0;

                if (desc.BindFlags & BIND_CONSTANT_BUFFER)
                {
                    barrier.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
                }
                if (desc.BindFlags & BIND_VERTEX_BUFFER)
                {
                    barrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
                }
                if (desc.BindFlags & BIND_INDEX_BUFFER)
                {
                    barrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
                }
                if (desc.BindFlags & BIND_SHADER_RESOURCE)
                {
                    barrier.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT;
                }
                if (desc.BindFlags & BIND_UNORDERED_ACCESS)
                {
                    barrier.dstAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
                }

                // transfer queue-ownership from copy to graphics:
                barrier.srcQueueFamilyIndex = queueIndices.copyFamily;
                barrier.dstQueueFamilyIndex = queueIndices.graphicsFamily;

                vkCmdPipelineBarrier(
                    frame.copyCommandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0,
                    0, nullptr,
                    1, &barrier,
                    0, nullptr);
            }
            copyQueueLock.unlock();
        }

        // Create resource views if needed
        if (desc.BindFlags & BIND_SHADER_RESOURCE)
        {
            CreateSubresource(result, SRV, 0);
        }
        if (desc.BindFlags & BIND_UNORDERED_ACCESS)
        {
            CreateSubresource(result, UAV, 0);
        }

        return result;
    }

    bool GraphicsDevice_Vulkan::CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture)
    {
        auto internal_state = std::make_shared<Texture_Vulkan>();
        internal_state->allocationhandler = allocationhandler;
        pTexture->internal_state = internal_state;
        pTexture->type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;

        pTexture->desc = *pDesc;

        if (pTexture->desc.MipLevels == 0)
        {
            pTexture->desc.MipLevels = (uint32_t) log2(std::max(pTexture->desc.Width, pTexture->desc.Height)) + 1;
        }

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.extent.width = pTexture->desc.Width;
        imageInfo.extent.height = pTexture->desc.Height;
        imageInfo.extent.depth = 1;
        imageInfo.format = _ConvertFormat(pTexture->desc.format);
        imageInfo.arrayLayers = pTexture->desc.ArraySize;
        imageInfo.mipLevels = pTexture->desc.MipLevels;
        imageInfo.samples = (VkSampleCountFlagBits) pTexture->desc.SampleCount;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = 0;
        if (pTexture->desc.BindFlags & BIND_SHADER_RESOURCE)
        {
            imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (pTexture->desc.BindFlags & BIND_UNORDERED_ACCESS)
        {
            imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (pTexture->desc.BindFlags & BIND_RENDER_TARGET)
        {
            imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }
        if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
        {
            imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        imageInfo.flags = 0;
        if (pTexture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
        {
            imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        switch (pTexture->desc.type)
        {
            case TextureDesc::TEXTURE_1D:
                imageInfo.imageType = VK_IMAGE_TYPE_1D;
                break;
            case TextureDesc::TEXTURE_2D:
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                break;
            case TextureDesc::TEXTURE_3D:
                imageInfo.imageType = VK_IMAGE_TYPE_3D;
                imageInfo.extent.depth = pTexture->desc.Depth;
                break;
            default:
                assert(0);
                break;
        }

        VkResult res;

        if (pTexture->desc.Usage == USAGE_STAGING)
        {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = imageInfo.extent.width * imageInfo.extent.height * imageInfo.extent.depth * imageInfo.arrayLayers *
                              GetPixelFormatSize(pTexture->desc.format);

            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            if (pDesc->Usage == USAGE_STAGING)
            {
                if (pDesc->CPUAccessFlags & CPU_ACCESS_READ)
                {
                    allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
                    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                }
                else
                {
                    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
                    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                }
            }

            res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->staging_resource, &internal_state->allocation, nullptr);
            assert(res == VK_SUCCESS);

            imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
            VkImage image;
            res = vkCreateImage(device, &imageInfo, nullptr, &image);
            assert(res == VK_SUCCESS);

            VkImageSubresource subresource = {};
            subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vkGetImageSubresourceLayout(device, image, &subresource, &internal_state->subresourcelayout);

            vkDestroyImage(device, image, nullptr);
            return res == VK_SUCCESS;
        }
        else
        {
            res = vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &internal_state->resource, &internal_state->allocation, nullptr);
            assert(res == VK_SUCCESS);
        }

        // Issue data copy on request:
        if (pInitialData != nullptr)
        {
            GPUBufferDesc uploadBufferDesc = {};
            uploadBufferDesc.ByteWidth = (uint32_t) internal_state->allocation->GetSize();
            uploadBufferDesc.Usage = USAGE_STAGING;

            RefPtr<GraphicsBuffer> uploadBuffer = CreateBuffer(uploadBufferDesc, nullptr);
            assert(uploadBuffer.IsNotNull());

            VkBuffer upload_resource = to_internal(uploadBuffer.Get())->resource;
            VmaAllocation upload_allocation = to_internal(uploadBuffer.Get())->allocation;

            void* pData = upload_allocation->GetMappedData();
            assert(pData != nullptr);

            std::vector<VkBufferImageCopy> copyRegions;

            size_t cpyoffset = 0;
            uint32_t initDataIdx = 0;
            for (uint32_t slice = 0; slice < pDesc->ArraySize; ++slice)
            {
                uint32_t width = pDesc->Width;
                uint32_t height = pDesc->Height;
                for (uint32_t mip = 0; mip < pDesc->MipLevels; ++mip)
                {
                    const SubresourceData& subresourceData = pInitialData[initDataIdx++];
                    size_t cpysize = subresourceData.SysMemPitch * height;
                    if (IsFormatBlockCompressed(pDesc->format))
                    {
                        cpysize /= 4;
                    }
                    uint8_t* cpyaddr = (uint8_t*) pData + cpyoffset;
                    memcpy(cpyaddr, subresourceData.pSysMem, cpysize);

                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset = cpyoffset;
                    copyRegion.bufferRowLength = 0;
                    copyRegion.bufferImageHeight = 0;

                    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = mip;
                    copyRegion.imageSubresource.baseArrayLayer = slice;
                    copyRegion.imageSubresource.layerCount = 1;

                    copyRegion.imageOffset = {0, 0, 0};
                    copyRegion.imageExtent = {
                        width,
                        height,
                        1};

                    width = std::max(1u, width / 2);
                    height = std::max(1u, height / 2);

                    copyRegions.push_back(copyRegion);

                    cpyoffset += Align(cpysize, GetPixelFormatSize(pDesc->format));
                }
            }

            copyQueueLock.lock();
            {
                auto& frame = GetFrameResources();
                if (!copyQueueUse)
                {
                    copyQueueUse = true;

                    res = vkResetCommandPool(device, frame.copyCommandPool, 0);
                    assert(res == VK_SUCCESS);

                    VkCommandBufferBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                    beginInfo.pInheritanceInfo = nullptr; // Optional

                    res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
                    assert(res == VK_SUCCESS);
                }

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = internal_state->resource;
                barrier.oldLayout = imageInfo.initialLayout;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = imageInfo.arrayLayers;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = imageInfo.mipLevels;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                vkCmdPipelineBarrier(
                    frame.copyCommandBuffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                vkCmdCopyBufferToImage(frame.copyCommandBuffer, upload_resource, internal_state->resource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t) copyRegions.size(), copyRegions.data());

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = _ConvertImageLayout(pTexture->desc.layout);
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

                frame.loadedimagetransitions.push_back(barrier);
            }
            copyQueueLock.unlock();
        }
        else
        {
            copyQueueLock.lock();

            auto& frame = GetFrameResources();
            if (!copyQueueUse)
            {
                copyQueueUse = true;

                res = vkResetCommandPool(device, frame.copyCommandPool, 0);
                assert(res == VK_SUCCESS);

                VkCommandBufferBeginInfo beginInfo = {};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                beginInfo.pInheritanceInfo = nullptr; // Optional

                res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
                assert(res == VK_SUCCESS);
            }

            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = internal_state->resource;
            barrier.oldLayout = imageInfo.initialLayout;
            barrier.newLayout = _ConvertImageLayout(pTexture->desc.layout);
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                if (IsFormatStencilSupport(pTexture->desc.format))
                {
                    barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }
            else
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = imageInfo.arrayLayers;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = imageInfo.mipLevels;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            frame.loadedimagetransitions.push_back(barrier);

            copyQueueLock.unlock();
        }

        if (pTexture->desc.BindFlags & BIND_RENDER_TARGET)
        {
            CreateSubresource(pTexture, RTV, 0, -1, 0, -1);
        }
        if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
        {
            CreateSubresource(pTexture, DSV, 0, -1, 0, -1);
        }
        if (pTexture->desc.BindFlags & BIND_SHADER_RESOURCE)
        {
            CreateSubresource(pTexture, SRV, 0, -1, 0, -1);
        }
        if (pTexture->desc.BindFlags & BIND_UNORDERED_ACCESS)
        {
            CreateSubresource(pTexture, UAV, 0, -1, 0, -1);
        }

        return res == VK_SUCCESS;
    }

    bool GraphicsDevice_Vulkan::CreateShader(ShaderStage stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader)
    {
        auto internal_state = std::make_shared<Shader_Vulkan>();
        internal_state->allocationhandler = allocationhandler;
        pShader->internal_state = internal_state;

        pShader->code.resize(BytecodeLength);
        std::memcpy(pShader->code.data(), pShaderBytecode, BytecodeLength);
        pShader->stage = stage;

        VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        moduleInfo.codeSize = pShader->code.size();
        moduleInfo.pCode = (uint32_t*) pShader->code.data();

        VK_CHECK(vkCreateShaderModule(device, &moduleInfo, nullptr, &internal_state->shaderModule));

        internal_state->stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        internal_state->stageInfo.module = internal_state->shaderModule;
        internal_state->stageInfo.pName = "main";
        switch (stage)
        {
            case ShaderStage::Mesh:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_MESH_BIT_NV;
                break;
            case ShaderStage::Amplification:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_TASK_BIT_NV;
                break;
            case ShaderStage::Vertex:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderStage::Hull:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case ShaderStage::Domain:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case ShaderStage::Geometry:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case ShaderStage::Fragment:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case ShaderStage::Compute:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                internal_state->stageInfo.stage = VK_SHADER_STAGE_ALL;
                // library shader (ray tracing)
                break;
        }

        if (pShader->rootSignature == nullptr)
        {
            // Perform shader reflection for shaders that don't specify a root signature:
            spirv_cross::Compiler comp((uint32_t*) pShader->code.data(), pShader->code.size() / sizeof(uint32_t));
            auto entrypoints = comp.get_entry_points_and_stages();
            auto active = comp.get_active_interface_variables();
            spirv_cross::ShaderResources resources = comp.get_shader_resources(active);
            comp.set_enabled_interface_variables(move(active));

            internal_state->entrypoints.reserve(entrypoints.size());
            for (auto& x : entrypoints)
            {
                internal_state->entrypoints.push_back(x);
            }

            std::vector<VkDescriptorSetLayoutBinding>& layoutBindings = internal_state->layoutBindings;
            std::vector<VkImageViewType>& imageViewTypes = internal_state->imageViewTypes;

            for (auto& x : resources.separate_samplers)
            {
                VkDescriptorSetLayoutBinding layoutBinding = {};
                layoutBinding.stageFlags = internal_state->stageInfo.stage;
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
                layoutBinding.descriptorCount = 1;
                layoutBindings.push_back(layoutBinding);
                imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
            }
            for (auto& x : resources.separate_images)
            {
                VkDescriptorSetLayoutBinding layoutBinding = {};
                layoutBinding.stageFlags = internal_state->stageInfo.stage;
                auto image = comp.get_type_from_variable(x.id).image;
                switch (image.dim)
                {
                    case spv::Dim1D:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        if (image.arrayed)
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_1D_ARRAY);
                        }
                        else
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_1D);
                        }
                        break;
                    case spv::Dim2D:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        if (image.arrayed)
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_2D_ARRAY);
                        }
                        else
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_2D);
                        }
                        break;
                    case spv::Dim3D:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_3D);
                        break;
                    case spv::DimCube:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        if (image.arrayed)
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY);
                        }
                        else
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_CUBE);
                        }
                        break;
                    case spv::DimBuffer:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                        imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
                        break;
                    default:
                        imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
                        break;
                }
                layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
                layoutBinding.descriptorCount = 1;
                layoutBindings.push_back(layoutBinding);
            }
            for (auto& x : resources.storage_images)
            {
                VkDescriptorSetLayoutBinding layoutBinding = {};
                layoutBinding.stageFlags = internal_state->stageInfo.stage;
                auto image = comp.get_type_from_variable(x.id).image;
                switch (image.dim)
                {
                    case spv::Dim1D:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        if (image.arrayed)
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_1D_ARRAY);
                        }
                        else
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_1D);
                        }
                        break;
                    case spv::Dim2D:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        if (image.arrayed)
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_2D_ARRAY);
                        }
                        else
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_2D);
                        }
                        break;
                    case spv::Dim3D:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_3D);
                        break;
                    case spv::DimCube:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        if (image.arrayed)
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY);
                        }
                        else
                        {
                            imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_CUBE);
                        }
                        break;
                    case spv::DimBuffer:
                        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                        imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
                        break;
                    default:
                        imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
                        break;
                }
                layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
                layoutBinding.descriptorCount = 1;
                layoutBindings.push_back(layoutBinding);
            }
            for (auto& x : resources.uniform_buffers)
            {
                VkDescriptorSetLayoutBinding layoutBinding = {};
                layoutBinding.stageFlags = internal_state->stageInfo.stage;
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
                layoutBinding.descriptorCount = 1;
                layoutBindings.push_back(layoutBinding);
                imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
            }
            for (auto& x : resources.storage_buffers)
            {
                VkDescriptorSetLayoutBinding layoutBinding = {};
                layoutBinding.stageFlags = internal_state->stageInfo.stage;
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
                layoutBinding.descriptorCount = 1;
                layoutBindings.push_back(layoutBinding);
                imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
            }
            for (auto& x : resources.acceleration_structures)
            {
                VkDescriptorSetLayoutBinding layoutBinding = {};
                layoutBinding.stageFlags = internal_state->stageInfo.stage;
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
                layoutBinding.descriptorCount = 1;
                layoutBindings.push_back(layoutBinding);
                imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
            }

            if (stage == ShaderStage::Compute || stage == ShaderStage::Count)
            {
                VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo = {};
                descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                descriptorSetlayoutInfo.pBindings = layoutBindings.data();
                descriptorSetlayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
                VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &internal_state->descriptorSetLayout));

                VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.pSetLayouts = &internal_state->descriptorSetLayout;
                pipelineLayoutInfo.setLayoutCount = 1; // cs
                pipelineLayoutInfo.pushConstantRangeCount = 0;

                VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internal_state->pipelineLayout_cs));
            }
        }

        if (stage == ShaderStage::Compute)
        {
            VkComputePipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            if (pShader->rootSignature == nullptr)
            {
                // pipeline layout from reflection:
                pipelineInfo.layout = internal_state->pipelineLayout_cs;
            }
            else
            {
                // pipeline layout from root signature:
                pipelineInfo.layout = to_internal(pShader->rootSignature)->pipelineLayout;
            }
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

            // Create compute pipeline state in place:
            pipelineInfo.stage = internal_state->stageInfo;

            VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &internal_state->pipeline_cs));
        }

        return true;
    }

    bool GraphicsDevice_Vulkan::CreateShader(ShaderStage stage, const char* source, const char* entryPoint, Shader* pShader)
    {
#if !defined(ALIMER_DISABLE_SHADER_COMPILER) && defined(VK_USE_PLATFORM_WIN32_KHR)
        IDxcLibrary* dxcLibrary = GetOrCreateDxcLibrary();
        IDxcCompiler* dxcCompiler = GetOrCreateDxcCompiler();

        IDxcIncludeHandler* includeHandler;
        dxcLibrary->CreateIncludeHandler(&includeHandler);

        IDxcBlobEncoding* sourceBlob;
        dxcLibrary->CreateBlobWithEncodingOnHeapCopy(source, (UINT32) strlen(source), CP_UTF8, &sourceBlob);

        std::wstring entryPointW = ToUtf16(entryPoint);
        std::vector<const wchar_t*> arguments;
        arguments.push_back(L"/Zpc"); // Column major
#    ifdef _DEBUG
        //arguments.push_back(L"/Zi");
#    else
        arguments.push_back(L"/O3");
#    endif
        arguments.push_back(L"-spirv");
        arguments.push_back(L"-fspv-target-env=vulkan1.2");
        arguments.push_back(L"-fvk-use-dx-layout");
        arguments.push_back(L"-flegacy-macro-expansion");

        if (stage == ShaderStage::Vertex || stage == ShaderStage::Domain || stage == ShaderStage::Geometry)
        {
            arguments.push_back(L"-fvk-invert-y");
        }

        arguments.push_back(L"-fvk-t-shift");
        arguments.push_back(L"1000");
        arguments.push_back(L"all");

        arguments.push_back(L"-fvk-u-shift");
        arguments.push_back(L"2000");
        arguments.push_back(L"all");

        arguments.push_back(L"-fvk-s-shift");
        arguments.push_back(L"3000");
        arguments.push_back(L"all");

        const wchar_t* target = L"vs_6_1";
        switch (stage)
        {
            case ShaderStage::Hull:
                target = L"hs_6_1";
                break;
            case ShaderStage::Domain:
                target = L"ds_6_1";
                break;
            case ShaderStage::Geometry:
                target = L"gs_6_1";
                break;
            case ShaderStage::Fragment:
                target = L"ps_6_1";
                break;
            case ShaderStage::Compute:
                target = L"cs_6_1";
                break;
        }

        IDxcOperationResult* compileResult;
        dxcCompiler->Compile(
            sourceBlob,
            nullptr,
            entryPointW.c_str(),
            target,
            arguments.data(),
            (UINT32) arguments.size(),
            nullptr,
            0,
            includeHandler,
            &compileResult);

        HRESULT hr;
        compileResult->GetStatus(&hr);

        if (FAILED(hr))
        {
            IDxcBlobEncoding* errors;
            compileResult->GetErrorBuffer(&errors);
            std::string message = std::string("DXC compile failed with ") + static_cast<char*>(errors->GetBufferPointer());
            LOGE("{}", message);
            errors->Release();
            return false;
        }

        IDxcBlob* compiledShader;
        compileResult->GetResult(&compiledShader);

        bool result = CreateShader(stage, compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), pShader);
        if (result)
        {
            to_internal(pShader)->stageInfo.pName = entryPoint;
        }

        sourceBlob->Release();
        compiledShader->Release();
        compileResult->Release();
        return result;
#else
        pShader->internal_state = nullptr;
        return false;
#endif
    }

    RefPtr<Sampler> GraphicsDevice_Vulkan::CreateSampler(const SamplerDescriptor* descriptor)
    {
        RefPtr<Sampler_Vulkan> result(new Sampler_Vulkan());
        result->allocationhandler = allocationhandler;

        VkSamplerCreateInfo createInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        createInfo.flags = 0;
        createInfo.pNext = nullptr;
        createInfo.magFilter = _ConvertFilter(descriptor->magFilter);
        createInfo.minFilter = _ConvertFilter(descriptor->minFilter);
        createInfo.mipmapMode = _ConvertMipMapFilterMode(descriptor->mipmapFilter);
        createInfo.addressModeU = _ConvertAddressMode(descriptor->addressModeU);
        createInfo.addressModeV = _ConvertAddressMode(descriptor->addressModeV);
        createInfo.addressModeW = _ConvertAddressMode(descriptor->addressModeW);
        createInfo.mipLodBias = descriptor->mipLodBias;
        createInfo.anisotropyEnable = (descriptor->maxAnisotropy > 1) ? VK_TRUE : VK_FALSE;
        createInfo.maxAnisotropy = static_cast<float>(descriptor->maxAnisotropy);
        if (descriptor->compareFunction != CompareFunction::Undefined)
        {
            createInfo.compareEnable = VK_TRUE;
            createInfo.compareOp = _ConvertComparisonFunc(descriptor->compareFunction);
        }
        else
        {
            createInfo.compareEnable = VK_FALSE;
            createInfo.compareOp = VK_COMPARE_OP_NEVER;
        }

        createInfo.minLod = descriptor->lodMinClamp;
        createInfo.maxLod = descriptor->lodMaxClamp;
        createInfo.borderColor = _ConvertSamplerBorderColor(descriptor->borderColor);
        createInfo.unnormalizedCoordinates = VK_FALSE;

        VkResult res = vkCreateSampler(device, &createInfo, nullptr, &result->resource);
        if (res != VK_SUCCESS)
        {
            return nullptr;
        }

        return result;
    }

    bool GraphicsDevice_Vulkan::CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery)
    {
        auto internal_state = std::make_shared<Query_Vulkan>();
        internal_state->allocationhandler = allocationhandler;
        pQuery->internal_state = internal_state;

        bool hr = false;

        pQuery->desc = *pDesc;
        internal_state->query_type = pQuery->desc.Type;

        switch (pDesc->Type)
        {
            case GPU_QUERY_TYPE_TIMESTAMP:
                if (allocationhandler->free_timestampqueries.pop_front(internal_state->query_index))
                {
                    hr = true;
                }
                else
                {
                    internal_state->query_type = GPU_QUERY_TYPE_INVALID;
                    assert(0);
                }
                break;
            case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
                hr = true;
                break;
            case GPU_QUERY_TYPE_OCCLUSION:
            case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
                if (allocationhandler->free_occlusionqueries.pop_front(internal_state->query_index))
                {
                    hr = true;
                }
                else
                {
                    internal_state->query_type = GPU_QUERY_TYPE_INVALID;
                    assert(0);
                }
                break;
        }

        assert(hr);

        return hr;
    }

    bool GraphicsDevice_Vulkan::CreateRenderPipelineCore(const RenderPipelineDescriptor* descriptor, RenderPipeline** pipeline)
    {
        RefPtr<PipelineState_Vulkan> internal_state(new PipelineState_Vulkan());
        internal_state->allocationhandler = allocationhandler;
        internal_state->desc = *descriptor;

        internal_state->hash = 0;
        alimer::CombineHash(internal_state->hash, descriptor->ms);
        alimer::CombineHash(internal_state->hash, descriptor->as);
        alimer::CombineHash(internal_state->hash, descriptor->vs);
        alimer::CombineHash(internal_state->hash, descriptor->ps);
        alimer::CombineHash(internal_state->hash, descriptor->hs);
        alimer::CombineHash(internal_state->hash, descriptor->ds);
        alimer::CombineHash(internal_state->hash, descriptor->gs);
        //alimer::CombineHash(internal_state->hash, descriptor->bs);
        alimer::CombineHash(internal_state->hash, descriptor->sampleMask);
        alimer::CombineHash(internal_state->hash, descriptor->rasterizationState);
        alimer::CombineHash(internal_state->hash, descriptor->depthStencilState);
        alimer::CombineHash(internal_state->hash, descriptor->vertexDescriptor);
        alimer::CombineHash(internal_state->hash, descriptor->primitiveTopology);

        if (descriptor->rootSignature == nullptr)
        {
            // Descriptor set layout comes from reflection data when there is no root signature specified:

            auto insert_shader = [&](const Shader* shader) {
                if (shader == nullptr)
                    return;
                auto shader_internal = to_internal(shader);

                uint32_t i = 0;
                size_t check_max = internal_state->layoutBindings.size(); // dont't check for duplicates within self table
                for (auto& x : shader_internal->layoutBindings)
                {
                    bool found = false;
                    size_t j = 0;
                    for (auto& y : internal_state->layoutBindings)
                    {
                        if (x.binding == y.binding)
                        {
                            // If the asserts fire, it means there are overlapping bindings between shader stages
                            //	This is not supported now for performance reasons (less binding management)!
                            //	(Overlaps between s/b/t bind points are not a problem because those are shifted by the compiler)
                            assert(x.descriptorCount == y.descriptorCount);
                            assert(x.descriptorType == y.descriptorType);
                            found = true;
                            y.stageFlags |= x.stageFlags;
                            break;
                        }
                        if (j++ >= check_max)
                            break;
                    }

                    if (!found)
                    {
                        internal_state->layoutBindings.push_back(x);
                        internal_state->imageViewTypes.push_back(shader_internal->imageViewTypes[i]);
                    }
                    i++;
                }
            };

            insert_shader(descriptor->ms);
            insert_shader(descriptor->as);
            insert_shader(descriptor->vs);
            insert_shader(descriptor->hs);
            insert_shader(descriptor->ds);
            insert_shader(descriptor->gs);
            insert_shader(descriptor->ps);

            VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            descriptorSetlayoutInfo.pBindings = internal_state->layoutBindings.data();
            descriptorSetlayoutInfo.bindingCount = static_cast<uint32_t>(internal_state->layoutBindings.size());
            VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &internal_state->descriptorSetLayout));

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            pipelineLayoutInfo.pSetLayouts = &internal_state->descriptorSetLayout;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pushConstantRangeCount = 0;

            VK_CHECK(
                vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internal_state->pipelineLayout));
        }

        *pipeline = internal_state.Detach();
        return true;
    }

    bool GraphicsDevice_Vulkan::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
    {
        auto internal_state = std::make_shared<RenderPass_Vulkan>();
        internal_state->allocationhandler = allocationhandler;
        renderpass->internal_state = internal_state;

        renderpass->desc = *pDesc;

        renderpass->hash = 0;
        alimer::CombineHash(renderpass->hash, pDesc->attachments.size());
        for (auto& attachment : pDesc->attachments)
        {
            alimer::CombineHash(renderpass->hash, attachment.texture->desc.format);
            alimer::CombineHash(renderpass->hash, attachment.texture->desc.SampleCount);
        }

        VkResult res;

        VkImageView attachments[17] = {};
        VkAttachmentDescription attachmentDescriptions[17] = {};
        VkAttachmentReference colorAttachmentRefs[8] = {};
        VkAttachmentReference resolveAttachmentRefs[8] = {};
        VkAttachmentReference depthAttachmentRef = {};

        int resolvecount = 0;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        const RenderPassDesc& desc = renderpass->desc;

        uint32_t validAttachmentCount = 0;
        for (auto& attachment : renderpass->desc.attachments)
        {
            const Texture* texture = attachment.texture;
            const TextureDesc& texdesc = texture->desc;
            int subresource = attachment.subresource;
            auto texture_internal_state = to_internal(texture);

            attachmentDescriptions[validAttachmentCount].format = _ConvertFormat(texdesc.format);
            attachmentDescriptions[validAttachmentCount].samples = (VkSampleCountFlagBits) texdesc.SampleCount;

            switch (attachment.loadop)
            {
                default:
                case RenderPassAttachment::LOADOP_LOAD:
                    attachmentDescriptions[validAttachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                    break;
                case RenderPassAttachment::LOADOP_CLEAR:
                    attachmentDescriptions[validAttachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    break;
                case RenderPassAttachment::LOADOP_DONTCARE:
                    attachmentDescriptions[validAttachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    break;
            }

            switch (attachment.storeop)
            {
                default:
                case RenderPassAttachment::STOREOP_STORE:
                    attachmentDescriptions[validAttachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    break;
                case RenderPassAttachment::STOREOP_DONTCARE:
                    attachmentDescriptions[validAttachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    break;
            }

            attachmentDescriptions[validAttachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescriptions[validAttachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            attachmentDescriptions[validAttachmentCount].initialLayout = _ConvertImageLayout(attachment.initial_layout);
            attachmentDescriptions[validAttachmentCount].finalLayout = _ConvertImageLayout(attachment.final_layout);

            if (attachment.type == RenderPassAttachment::RENDERTARGET)
            {
                if (subresource < 0 || texture_internal_state->subresources_rtv.empty())
                {
                    attachments[validAttachmentCount] = texture_internal_state->rtv;
                }
                else
                {
                    assert(texture_internal_state->subresources_rtv.size() > size_t(subresource) && "Invalid RTV subresource!");
                    attachments[validAttachmentCount] = texture_internal_state->subresources_rtv[subresource];
                }
                if (attachments[validAttachmentCount] == VK_NULL_HANDLE)
                {
                    continue;
                }

                colorAttachmentRefs[subpass.colorAttachmentCount].attachment = validAttachmentCount;
                colorAttachmentRefs[subpass.colorAttachmentCount].layout = _ConvertImageLayout(attachment.subpass_layout);
                subpass.colorAttachmentCount++;
                subpass.pColorAttachments = colorAttachmentRefs;
            }
            else if (attachment.type == RenderPassAttachment::DEPTH_STENCIL)
            {
                if (subresource < 0 || texture_internal_state->subresources_dsv.empty())
                {
                    attachments[validAttachmentCount] = texture_internal_state->dsv;
                }
                else
                {
                    assert(texture_internal_state->subresources_dsv.size() > size_t(subresource) && "Invalid DSV subresource!");
                    attachments[validAttachmentCount] = texture_internal_state->subresources_dsv[subresource];
                }
                if (attachments[validAttachmentCount] == VK_NULL_HANDLE)
                {
                    continue;
                }

                if (IsFormatStencilSupport(texdesc.format))
                {
                    switch (attachment.loadop)
                    {
                        default:
                        case RenderPassAttachment::LOADOP_LOAD:
                            attachmentDescriptions[validAttachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                            break;
                        case RenderPassAttachment::LOADOP_CLEAR:
                            attachmentDescriptions[validAttachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                            break;
                        case RenderPassAttachment::LOADOP_DONTCARE:
                            attachmentDescriptions[validAttachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                            break;
                    }

                    switch (attachment.storeop)
                    {
                        default:
                        case RenderPassAttachment::STOREOP_STORE:
                            attachmentDescriptions[validAttachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                            break;
                        case RenderPassAttachment::STOREOP_DONTCARE:
                            attachmentDescriptions[validAttachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                            break;
                    }
                }

                depthAttachmentRef.attachment = validAttachmentCount;
                depthAttachmentRef.layout = _ConvertImageLayout(attachment.subpass_layout);
                subpass.pDepthStencilAttachment = &depthAttachmentRef;
            }
            else if (attachment.type == RenderPassAttachment::RESOLVE)
            {
                if (attachment.texture == nullptr)
                {
                    resolveAttachmentRefs[resolvecount].attachment = VK_ATTACHMENT_UNUSED;
                }
                else
                {
                    if (subresource < 0 || texture_internal_state->subresources_srv.empty())
                    {
                        attachments[validAttachmentCount] = texture_internal_state->srv;
                    }
                    else
                    {
                        assert(texture_internal_state->subresources_srv.size() > size_t(subresource) && "Invalid SRV subresource!");
                        attachments[validAttachmentCount] = texture_internal_state->subresources_srv[subresource];
                    }
                    if (attachments[validAttachmentCount] == VK_NULL_HANDLE)
                    {
                        continue;
                    }
                    resolveAttachmentRefs[resolvecount].attachment = validAttachmentCount;
                    resolveAttachmentRefs[resolvecount].layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                }

                resolvecount++;
                subpass.pResolveAttachments = resolveAttachmentRefs;
            }

            validAttachmentCount++;
        }
        assert(renderpass->desc.attachments.size() == validAttachmentCount);

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = validAttachmentCount;
        renderPassInfo.pAttachments = attachmentDescriptions;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &internal_state->renderpass);
        assert(res == VK_SUCCESS);

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = internal_state->renderpass;
        framebufferInfo.attachmentCount = validAttachmentCount;
        framebufferInfo.pAttachments = attachments;

        if (validAttachmentCount > 0)
        {
            const TextureDesc& texdesc = desc.attachments[0].texture->desc;
            framebufferInfo.width = texdesc.Width;
            framebufferInfo.height = texdesc.Height;
            framebufferInfo.layers = texdesc.MiscFlags & RESOURCE_MISC_TEXTURECUBE ? 6 : 1; // todo figure out better! can't use ArraySize here, it will crash!
        }
        else
        {
            framebufferInfo.width = 1;
            framebufferInfo.height = 1;
            framebufferInfo.layers = 1;
        }

        res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &internal_state->framebuffer);
        assert(res == VK_SUCCESS);

        internal_state->beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        internal_state->beginInfo.renderPass = internal_state->renderpass;
        internal_state->beginInfo.framebuffer = internal_state->framebuffer;

        if (validAttachmentCount > 0)
        {
            const TextureDesc& texdesc = desc.attachments[0].texture->desc;

            internal_state->beginInfo.renderArea.offset = {0, 0};
            internal_state->beginInfo.renderArea.extent.width = texdesc.Width;
            internal_state->beginInfo.renderArea.extent.height = texdesc.Height;
            internal_state->beginInfo.clearValueCount = validAttachmentCount;
            internal_state->beginInfo.pClearValues = internal_state->clearColors;

            int i = 0;
            for (auto& attachment : desc.attachments)
            {
                if (desc.attachments[i].type == RenderPassAttachment::RESOLVE || attachment.texture == nullptr)
                    continue;

                const ClearValue& clear = desc.attachments[i].texture->desc.clear;
                if (desc.attachments[i].type == RenderPassAttachment::RENDERTARGET)
                {
                    internal_state->clearColors[i].color.float32[0] = clear.color[0];
                    internal_state->clearColors[i].color.float32[1] = clear.color[1];
                    internal_state->clearColors[i].color.float32[2] = clear.color[2];
                    internal_state->clearColors[i].color.float32[3] = clear.color[3];
                }
                else if (desc.attachments[i].type == RenderPassAttachment::DEPTH_STENCIL)
                {
                    internal_state->clearColors[i].depthStencil.depth = clear.depthstencil.depth;
                    internal_state->clearColors[i].depthStencil.stencil = clear.depthstencil.stencil;
                }
                else
                {
                    assert(0);
                }
                i++;
            }
        }

        return res == VK_SUCCESS;
    }

    bool GraphicsDevice_Vulkan::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh)
    {
        auto internal_state = std::make_shared<BVH_Vulkan>();
        internal_state->allocationhandler = allocationhandler;
        bvh->internal_state = internal_state;
        bvh->type = GPUResource::GPU_RESOURCE_TYPE::RAYTRACING_ACCELERATION_STRUCTURE;

        bvh->desc = *pDesc;

        VkAccelerationStructureCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;

        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
        {
            info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        }
        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
        {
            info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        }
        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
        {
            info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        }
        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
        {
            info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
        }
        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
        {
            info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
        }

        switch (pDesc->type)
        {
            case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
            {
                info.type = VkAccelerationStructureTypeKHR::VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

                for (auto& x : pDesc->bottomlevel.geometries)
                {
                    internal_state->geometries.emplace_back();
                    auto& geometry = internal_state->geometries.back();
                    geometry = {};
                    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;

                    if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
                    {
                        geometry.allowsTransforms = VK_TRUE;
                    }

                    if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
                    {
                        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                        geometry.maxPrimitiveCount = x.triangles.indexCount / 3;
                        geometry.indexType = (x.triangles.indexFormat == IndexFormat::UInt16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
                        geometry.maxVertexCount = x.triangles.vertexCount;
                        geometry.vertexFormat = _ConvertVertexFormat(x.triangles.vertexFormat);
                    }
                    else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
                    {
                        geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
                        geometry.maxPrimitiveCount = x.aabbs.count;
                    }
                }
            }
            break;
            case RaytracingAccelerationStructureDesc::TOPLEVEL:
            {
                info.type = VkAccelerationStructureTypeKHR::VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

                internal_state->geometries.emplace_back();
                auto& geometry = internal_state->geometries.back();
                geometry = {};
                geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
                geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
                geometry.allowsTransforms = VK_TRUE;
                geometry.maxPrimitiveCount = pDesc->toplevel.count;
            }
            break;
        }

        info.pGeometryInfos = internal_state->geometries.data();
        info.maxGeometryCount = (uint32_t) internal_state->geometries.size();
        internal_state->info = info;

        VkResult res = createAccelerationStructureKHR(device, &info, nullptr, &internal_state->resource);
        assert(res == VK_SUCCESS);

        VkAccelerationStructureMemoryRequirementsInfoKHR meminfo = {};
        meminfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
        meminfo.accelerationStructure = internal_state->resource;

        meminfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
        VkMemoryRequirements2 memrequirements = {};
        memrequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        getAccelerationStructureMemoryRequirementsKHR(device, &meminfo, &memrequirements);

        meminfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
        VkMemoryRequirements2 memrequirements_scratch_build = {};
        memrequirements_scratch_build.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        getAccelerationStructureMemoryRequirementsKHR(device, &meminfo, &memrequirements_scratch_build);

        meminfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_KHR;
        VkMemoryRequirements2 memrequirements_scratch_update = {};
        memrequirements_scratch_update.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        getAccelerationStructureMemoryRequirementsKHR(device, &meminfo, &memrequirements_scratch_update);

        // Main backing memory:
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = memrequirements.memoryRequirements.size +
                          std::max(memrequirements_scratch_build.memoryRequirements.size, memrequirements_scratch_update.memoryRequirements.size);
        bufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        assert(features_1_2.bufferDeviceAddress == VK_TRUE);
        bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        bufferInfo.flags = 0;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->buffer, &internal_state->allocation, nullptr);
        assert(res == VK_SUCCESS);

        VkBindAccelerationStructureMemoryInfoKHR bind_info = {};
        bind_info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
        bind_info.accelerationStructure = internal_state->resource;
        bind_info.memory = internal_state->allocation->GetMemory();
        res = bindAccelerationStructureMemoryKHR(device, 1, &bind_info);
        assert(res == VK_SUCCESS);

        VkAccelerationStructureDeviceAddressInfoKHR addrinfo = {};
        addrinfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addrinfo.accelerationStructure = internal_state->resource;
        internal_state->as_address = getAccelerationStructureDeviceAddressKHR(device, &addrinfo);

        internal_state->scratch_offset = memrequirements.memoryRequirements.size;

        return res == VK_SUCCESS;
    }
    bool GraphicsDevice_Vulkan::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso)
    {
        auto internal_state = std::make_shared<RTPipelineState_Vulkan>();
        internal_state->allocationhandler = allocationhandler;
        rtpso->internal_state = internal_state;

        rtpso->desc = *pDesc;

        VkRayTracingPipelineCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        info.flags = 0;

        info.libraries.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;

        std::vector<VkPipelineShaderStageCreateInfo> stages;
        for (auto& x : pDesc->shaderlibraries)
        {
            stages.emplace_back();
            auto& stage = stages.back();
            stage = {};
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.module = to_internal(x.shader)->shaderModule;
            switch (x.type)
            {
                default:
                case ShaderLibrary::RAYGENERATION:
                    stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                    break;
                case ShaderLibrary::MISS:
                    stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
                    break;
                case ShaderLibrary::CLOSESTHIT:
                    stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                    break;
                case ShaderLibrary::ANYHIT:
                    stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                    break;
                case ShaderLibrary::INTERSECTION:
                    stage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                    break;
            }
            stage.pName = x.function_name.c_str();
        }
        info.stageCount = (uint32_t) stages.size();
        info.pStages = stages.data();

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
        groups.reserve(pDesc->hitgroups.size());
        for (auto& x : pDesc->hitgroups)
        {
            groups.emplace_back();
            auto& group = groups.back();
            group = {};
            group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            switch (x.type)
            {
                default:
                case ShaderHitGroup::GENERAL:
                    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                    break;
                case ShaderHitGroup::TRIANGLES:
                    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                    break;
                case ShaderHitGroup::PROCEDURAL:
                    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
                    break;
            }
            group.generalShader = x.general_shader;
            group.closestHitShader = x.closesthit_shader;
            group.anyHitShader = x.anyhit_shader;
            group.intersectionShader = x.intersection_shader;
        }
        info.groupCount = (uint32_t) groups.size();
        info.pGroups = groups.data();

        info.maxRecursionDepth = pDesc->max_trace_recursion_depth;

        if (pDesc->rootSignature == nullptr)
        {
            info.layout = to_internal(pDesc->shaderlibraries.front().shader)->pipelineLayout_cs; // think better way
        }
        else
        {
            info.layout = to_internal(pDesc->rootSignature)->pipelineLayout;
        }

        VkRayTracingPipelineInterfaceCreateInfoKHR library_interface = {};
        library_interface.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR;
        library_interface.maxPayloadSize = pDesc->max_payload_size_in_bytes;
        library_interface.maxAttributeSize = pDesc->max_attribute_size_in_bytes;
        library_interface.maxCallableSize = 0;

        info.basePipelineHandle = VK_NULL_HANDLE;
        info.basePipelineIndex = 0;

        VkResult res = createRayTracingPipelinesKHR(device, VK_NULL_HANDLE, 1, &info, nullptr, &internal_state->pipeline);
        assert(res == VK_SUCCESS);

        return res == VK_SUCCESS;
    }
    bool GraphicsDevice_Vulkan::CreateDescriptorTable(DescriptorTable* table)
    {
        auto internal_state = std::make_shared<DescriptorTable_Vulkan>();
        internal_state->allocationhandler = allocationhandler;
        table->internal_state = internal_state;

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(table->samplers.size() + table->resources.size() + table->staticsamplers.size());

        std::vector<VkDescriptorUpdateTemplateEntry> entries;
        entries.reserve(table->samplers.size() + table->resources.size());

        internal_state->descriptors.reserve(table->samplers.size() + table->resources.size());

        size_t offset = 0;
        for (auto& x : table->resources)
        {
            bindings.emplace_back();
            auto& binding = bindings.back();
            binding = {};
            binding.stageFlags = _ConvertStageFlags(table->stage);
            binding.descriptorCount = x.count;

            switch (x.binding)
            {
                case ROOT_CONSTANTBUFFER:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_B;
                    break;
                case ROOT_RAWBUFFER:
                case ROOT_STRUCTUREDBUFFER:
                case ROOT_RWRAWBUFFER:
                case ROOT_RWSTRUCTUREDBUFFER:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
                    break;

                case CONSTANTBUFFER:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_B;
                    break;
                case RAWBUFFER:
                case STRUCTUREDBUFFER:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
                    break;
                case TYPEDBUFFER:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
                    break;
                case TEXTURE1D:
                case TEXTURE1DARRAY:
                case TEXTURE2D:
                case TEXTURE2DARRAY:
                case TEXTURECUBE:
                case TEXTURECUBEARRAY:
                case TEXTURE3D:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
                    break;
                case ACCELERATIONSTRUCTURE:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
                    break;
                case RWRAWBUFFER:
                case RWSTRUCTUREDBUFFER:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_U;
                    break;
                case RWTYPEDBUFFER:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_U;
                    break;
                case RWTEXTURE1D:
                case RWTEXTURE1DARRAY:
                case RWTEXTURE2D:
                case RWTEXTURE2DARRAY:
                case RWTEXTURE3D:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    binding.binding = x.slot + VULKAN_BINDING_SHIFT_U;
                    break;
                default:
                    assert(0);
                    break;
            }

            // Unroll, because we need the ability to update an array element individually:
            internal_state->resource_write_remap.push_back(entries.size());
            for (uint32_t i = 0; i < binding.descriptorCount; ++i)
            {
                internal_state->descriptors.emplace_back();
                entries.emplace_back();
                auto& entry = entries.back();
                entry = {};
                entry.descriptorCount = 1;
                entry.descriptorType = binding.descriptorType;
                entry.dstArrayElement = i;
                entry.dstBinding = binding.binding;
                entry.offset = offset;
                entry.stride = sizeof(DescriptorTable_Vulkan::Descriptor);

                offset += entry.stride;
            }
        }
        for (auto& x : table->samplers)
        {
            internal_state->descriptors.emplace_back();
            bindings.emplace_back();
            auto& binding = bindings.back();
            binding = {};
            binding.stageFlags = _ConvertStageFlags(table->stage);
            binding.descriptorCount = x.count;
            binding.binding = x.slot + VULKAN_BINDING_SHIFT_S;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

            // Unroll, because we need the ability to update an array element individually:
            internal_state->sampler_write_remap.push_back(entries.size());
            for (uint32_t i = 0; i < binding.descriptorCount; ++i)
            {
                entries.emplace_back();
                auto& entry = entries.back();
                entry = {};
                entry.descriptorCount = 1;
                entry.descriptorType = binding.descriptorType;
                entry.dstArrayElement = i;
                entry.dstBinding = binding.binding;
                entry.offset = offset;
                entry.stride = sizeof(DescriptorTable_Vulkan::Descriptor);

                offset += entry.stride;
            }
        }

        std::vector<VkSampler> immutableSamplers;
        immutableSamplers.reserve(table->staticsamplers.size());

        for (auto& x : table->staticsamplers)
        {
            immutableSamplers.emplace_back();
            auto& immutablesampler = immutableSamplers.back();
            immutablesampler = to_internal(x.sampler)->resource;

            bindings.emplace_back();
            auto& binding = bindings.back();
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            binding.stageFlags = VK_SHADER_STAGE_ALL;
            binding.binding = x.slot + VULKAN_BINDING_SHIFT_S;
            binding.descriptorCount = 1;
            binding.pImmutableSamplers = &immutablesampler;
        }

        VkDescriptorSetLayoutCreateInfo layoutinfo = {};
        layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutinfo.flags = 0;
        layoutinfo.pBindings = bindings.data();
        layoutinfo.bindingCount = (uint32_t) bindings.size();
        VkResult res = vkCreateDescriptorSetLayout(device, &layoutinfo, nullptr, &internal_state->layout);
        assert(res == VK_SUCCESS);

        VkDescriptorUpdateTemplateCreateInfo updatetemplateinfo = {};
        updatetemplateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO;
        updatetemplateinfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;
        updatetemplateinfo.flags = 0;
        updatetemplateinfo.descriptorSetLayout = internal_state->layout;
        updatetemplateinfo.pDescriptorUpdateEntries = entries.data();
        updatetemplateinfo.descriptorUpdateEntryCount = (uint32_t) entries.size();
        res = vkCreateDescriptorUpdateTemplate(device, &updatetemplateinfo, nullptr, &internal_state->updatetemplate);
        assert(res == VK_SUCCESS);

        uint32_t slot = 0;
        for (auto& x : table->resources)
        {
            for (uint32_t i = 0; i < x.count; ++i)
            {
                WriteDescriptor(table, slot, i, (const GPUResource*) nullptr);
            }
            slot++;
        }
        slot = 0;
        for (auto& x : table->samplers)
        {
            for (uint32_t i = 0; i < x.count; ++i)
            {
                WriteDescriptor(table, slot, i, (const Sampler*) nullptr);
            }
            slot++;
        }

        return res == VK_SUCCESS;
    }

    bool GraphicsDevice_Vulkan::CreateRootSignature(RootSignature* rootsig)
    {
        auto internal_state = std::make_shared<RootSignature_Vulkan>();
        internal_state->allocationhandler = allocationhandler;
        rootsig->internal_state = internal_state;

        std::vector<VkDescriptorSetLayout> layouts;
        layouts.reserve(rootsig->tables.size());
        uint32_t space = 0;
        for (auto& x : rootsig->tables)
        {
            layouts.push_back(to_internal(&x)->layout);

            uint32_t rangeIndex = 0;
            for (auto& binding : x.resources)
            {
                if (binding.binding < CONSTANTBUFFER)
                {
                    assert(binding.count == 1); // descriptor array not allowed in the root
                    internal_state->root_remap.emplace_back();
                    internal_state->root_remap.back().space = space;
                    internal_state->root_remap.back().binding = binding.slot;
                    internal_state->root_remap.back().rangeIndex = rangeIndex;
                }
                rangeIndex++;
            }
            space++;
        }

        for (uint32_t cmd = 0; cmd < kCommandListCount; ++cmd)
        {
            internal_state->last_tables[cmd].resize(layouts.size());
            internal_state->last_descriptorsets[cmd].resize(layouts.size());

            for (auto& x : rootsig->tables)
            {
                for (auto& binding : x.resources)
                {
                    if (binding.binding < CONSTANTBUFFER)
                    {
                        internal_state->root_descriptors[cmd].emplace_back();
                        internal_state->root_offsets[cmd].emplace_back();
                    }
                }
            }
        }

        std::vector<VkPushConstantRange> pushranges;
        pushranges.reserve(rootsig->rootconstants.size());
        for (auto& x : rootsig->rootconstants)
        {
            pushranges.emplace_back();
            pushranges.back() = {};
            pushranges.back().stageFlags = _ConvertStageFlags(x.stage);
            pushranges.back().offset = 0;
            pushranges.back().size = x.size;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.setLayoutCount = (uint32_t) layouts.size();
        pipelineLayoutInfo.pPushConstantRanges = pushranges.data();
        pipelineLayoutInfo.pushConstantRangeCount = (uint32_t) pushranges.size();

        VkResult res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internal_state->pipelineLayout);
        assert(res == VK_SUCCESS);

        return res == VK_SUCCESS;
    }

    int GraphicsDevice_Vulkan::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
    {
        auto internal_state = to_internal(texture);

        VkImageViewCreateInfo view_desc = {};
        view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_desc.flags = 0;
        view_desc.image = internal_state->resource;
        view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_desc.subresourceRange.baseArrayLayer = firstSlice;
        view_desc.subresourceRange.layerCount = sliceCount;
        view_desc.subresourceRange.baseMipLevel = firstMip;
        view_desc.subresourceRange.levelCount = mipCount;
        view_desc.format = _ConvertFormat(texture->desc.format);

        if (texture->desc.type == TextureDesc::TEXTURE_1D)
        {
            if (texture->desc.ArraySize > 1)
            {
                view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            }
            else
            {
                view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D;
            }
        }
        else if (texture->desc.type == TextureDesc::TEXTURE_2D)
        {
            if (texture->desc.ArraySize > 1)
            {
                if (texture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
                {
                    if (texture->desc.ArraySize > 6 && sliceCount > 6)
                    {
                        view_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                    }
                    else
                    {
                        view_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
                    }
                }
                else
                {
                    view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                }
            }
            else
            {
                view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
            }
        }
        else if (texture->desc.type == TextureDesc::TEXTURE_3D)
        {
            view_desc.viewType = VK_IMAGE_VIEW_TYPE_3D;
        }

        switch (type)
        {
            case alimer::SRV:
            {
                /*switch (texture->desc.format)
                {
                    case PixelFormat::FORMAT_R16_TYPELESS:
                        view_desc.format = VK_FORMAT_D16_UNORM;
                        view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        break;
                    case PixelFormat::FORMAT_R32_TYPELESS:
                        view_desc.format = VK_FORMAT_D32_SFLOAT;
                        view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        break;
                    case PixelFormat::FORMAT_R24G8_TYPELESS:
                        view_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
                        view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        break;
                    case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                        view_desc.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                        view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        break;
                }*/

                VkImageView srv;
                VkResult res = vkCreateImageView(device, &view_desc, nullptr, &srv);

                if (res == VK_SUCCESS)
                {
                    if (internal_state->srv == VK_NULL_HANDLE)
                    {
                        internal_state->srv = srv;
                        return -1;
                    }
                    internal_state->subresources_srv.push_back(srv);
                    return int(internal_state->subresources_srv.size() - 1);
                }
                else
                {
                    assert(0);
                }
            }
            break;
            case alimer::UAV:
            {
                if (view_desc.viewType == VK_IMAGE_VIEW_TYPE_CUBE || view_desc.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
                {
                    view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                }

                VkImageView uav;
                VkResult res = vkCreateImageView(device, &view_desc, nullptr, &uav);

                if (res == VK_SUCCESS)
                {
                    if (internal_state->uav == VK_NULL_HANDLE)
                    {
                        internal_state->uav = uav;
                        return -1;
                    }
                    internal_state->subresources_uav.push_back(uav);
                    return int(internal_state->subresources_uav.size() - 1);
                }
                else
                {
                    assert(0);
                }
            }
            break;
            case alimer::RTV:
            {
                VkImageView rtv;
                view_desc.subresourceRange.levelCount = 1;
                VkResult res = vkCreateImageView(device, &view_desc, nullptr, &rtv);

                if (res == VK_SUCCESS)
                {
                    if (internal_state->rtv == VK_NULL_HANDLE)
                    {
                        internal_state->rtv = rtv;
                        return -1;
                    }
                    internal_state->subresources_rtv.push_back(rtv);
                    return int(internal_state->subresources_rtv.size() - 1);
                }
                else
                {
                    assert(0);
                }
            }
            break;
            case alimer::DSV:
            {
                view_desc.subresourceRange.levelCount = 1;
                view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                /*switch (texture->desc.format)
                {
                    case PixelFormat::FORMAT_R16_TYPELESS:
                        view_desc.format = VK_FORMAT_D16_UNORM;
                        break;
                    case PixelFormat::FORMAT_R32_TYPELESS:
                        view_desc.format = VK_FORMAT_D32_SFLOAT;
                        break;
                    case PixelFormat::FORMAT_R24G8_TYPELESS:
                        view_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
                        view_desc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                        break;
                    case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                        view_desc.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                        view_desc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                        break;
                }*/

                VkImageView dsv;
                VkResult res = vkCreateImageView(device, &view_desc, nullptr, &dsv);

                if (res == VK_SUCCESS)
                {
                    if (internal_state->dsv == VK_NULL_HANDLE)
                    {
                        internal_state->dsv = dsv;
                        return -1;
                    }
                    internal_state->subresources_dsv.push_back(dsv);
                    return int(internal_state->subresources_dsv.size() - 1);
                }
                else
                {
                    assert(0);
                }
            }
            break;
            default:
                break;
        }
        return -1;
    }
    int GraphicsDevice_Vulkan::CreateSubresource(GraphicsBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size)
    {
        auto internal_state = to_internal(buffer);
        const GPUBufferDesc& desc = buffer->GetDesc();
        VkResult res;

        switch (type)
        {
            case alimer::SRV:
            case alimer::UAV:
            {
                if (desc.format == PixelFormat::Invalid)
                {
                    return -1;
                }

                VkBufferViewCreateInfo srv_desc = {};
                srv_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
                srv_desc.buffer = internal_state->resource;
                srv_desc.flags = 0;
                srv_desc.format = _ConvertFormat(desc.format);
                srv_desc.offset = Align(offset, device_properties.properties.limits.minTexelBufferOffsetAlignment); // damn, if this needs alignment, that could break a lot of things! (index buffer, index offset?)
                srv_desc.range = std::min(size, (uint64_t) desc.ByteWidth - srv_desc.offset);

                VkBufferView view;
                res = vkCreateBufferView(device, &srv_desc, nullptr, &view);

                if (res == VK_SUCCESS)
                {
                    if (type == SRV)
                    {
                        if (internal_state->srv == VK_NULL_HANDLE)
                        {
                            internal_state->srv = view;
                            return -1;
                        }
                        internal_state->subresources_srv.push_back(view);
                        return int(internal_state->subresources_srv.size() - 1);
                    }
                    else
                    {
                        if (internal_state->uav == VK_NULL_HANDLE)
                        {
                            internal_state->uav = view;
                            return -1;
                        }
                        internal_state->subresources_uav.push_back(view);
                        return int(internal_state->subresources_uav.size() - 1);
                    }
                }
                else
                {
                    assert(0);
                }
            }
            break;
            default:
                assert(0);
                break;
        }
        return -1;
    }

    void GraphicsDevice_Vulkan::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest)
    {
        VkAccelerationStructureInstanceKHR* desc = (VkAccelerationStructureInstanceKHR*) dest;
        memcpy(&desc->transform, &instance->transform, sizeof(desc->transform));
        desc->instanceCustomIndex = instance->InstanceID;
        desc->mask = instance->InstanceMask;
        desc->instanceShaderBindingTableRecordOffset = instance->InstanceContributionToHitGroupIndex;
        desc->flags = instance->Flags;

        assert(instance->bottomlevel.IsAccelerationStructure());
        auto internal_state = to_internal((RaytracingAccelerationStructure*) &instance->bottomlevel);
        desc->accelerationStructureReference = internal_state->as_address;
    }
    void GraphicsDevice_Vulkan::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest)
    {
        VkResult res = getRayTracingShaderGroupHandlesKHR(device, to_internal(rtpso)->pipeline, group_index, 1, SHADER_IDENTIFIER_SIZE, dest);
        assert(res == VK_SUCCESS);
    }
    void GraphicsDevice_Vulkan::WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource, uint64_t offset)
    {
        auto table_internal = to_internal(table);
        size_t remap = table_internal->resource_write_remap[rangeIndex];
        auto& descriptor = table_internal->descriptors[remap + arrayIndex];

        switch (table->resources[rangeIndex].binding)
        {
            case CONSTANTBUFFER:
            case RAWBUFFER:
            case STRUCTUREDBUFFER:
            case ROOT_CONSTANTBUFFER:
            case ROOT_RAWBUFFER:
            case ROOT_STRUCTUREDBUFFER:
                if (resource == nullptr || !resource->IsValid())
                {
                    descriptor.bufferInfo.buffer = nullBuffer;
                    descriptor.bufferInfo.offset = 0;
                    descriptor.bufferInfo.range = VK_WHOLE_SIZE;
                }
                else if (resource->IsBuffer())
                {
                    const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
                    auto internal_state = to_internal(buffer);
                    descriptor.bufferInfo.buffer = internal_state->resource;
                    descriptor.bufferInfo.offset = offset;
                    descriptor.bufferInfo.range = VK_WHOLE_SIZE;
                }
                else
                {
                    assert(0);
                }
                break;
            case TYPEDBUFFER:
                if (resource == nullptr || !resource->IsValid())
                {
                    descriptor.bufferView = nullBufferView;
                }
                else if (resource->IsBuffer())
                {
                    const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
                    auto internal_state = to_internal(buffer);
                    descriptor.bufferView = subresource < 0 ? internal_state->srv : internal_state->subresources_srv[subresource];
                }
                else
                {
                    assert(0);
                }
                break;
            case TEXTURE1D:
            case TEXTURE1DARRAY:
            case TEXTURE2D:
            case TEXTURE2DARRAY:
            case TEXTURECUBE:
            case TEXTURECUBEARRAY:
            case TEXTURE3D:
                descriptor.imageinfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                descriptor.imageinfo.sampler = VK_NULL_HANDLE;
                if (resource == nullptr || !resource->IsValid())
                {
                    switch (table->resources[rangeIndex].binding)
                    {
                        case TEXTURE1D:
                            descriptor.imageinfo.imageView = nullImageView1D;
                            break;
                        case TEXTURE1DARRAY:
                            descriptor.imageinfo.imageView = nullImageView1DArray;
                            break;
                        case TEXTURE2D:
                            descriptor.imageinfo.imageView = nullImageView2D;
                            break;
                        case TEXTURE2DARRAY:
                            descriptor.imageinfo.imageView = nullImageView2DArray;
                            break;
                        case TEXTURECUBE:
                            descriptor.imageinfo.imageView = nullImageViewCube;
                            break;
                        case TEXTURECUBEARRAY:
                            descriptor.imageinfo.imageView = nullImageViewCubeArray;
                            break;
                        case TEXTURE3D:
                            descriptor.imageinfo.imageView = nullImageView3D;
                            break;
                    };
                }
                else if (resource->IsTexture())
                {
                    const Texture* texture = (const Texture*) resource;
                    auto internal_state = to_internal(texture);
                    if (subresource < 0)
                    {
                        descriptor.imageinfo.imageView = internal_state->srv;
                    }
                    else
                    {
                        descriptor.imageinfo.imageView = internal_state->subresources_srv[subresource];
                    }
                    VkImageLayout layout = _ConvertImageLayout(texture->desc.layout);
                    if (layout != VK_IMAGE_LAYOUT_GENERAL && layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                    {
                        // Means texture initial layout is not compatible, so it must have been transitioned
                        layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                    descriptor.imageinfo.imageLayout = layout;
                }
                else
                {
                    assert(0);
                }
                break;
            case ACCELERATIONSTRUCTURE:
                if (resource == nullptr || !resource->IsValid())
                {
                    // nothing
                }
                else if (resource->IsAccelerationStructure())
                {
                    auto internal_state = to_internal((const RaytracingAccelerationStructure*) resource);
                    descriptor.accelerationStructure = internal_state->resource;
                }
                else
                {
                    assert(0);
                }
                break;
            case RWRAWBUFFER:
            case RWSTRUCTUREDBUFFER:
            case ROOT_RWRAWBUFFER:
            case ROOT_RWSTRUCTUREDBUFFER:
                if (resource == nullptr || !resource->IsValid())
                {
                    descriptor.bufferInfo.buffer = nullBuffer;
                    descriptor.bufferInfo.offset = 0;
                    descriptor.bufferInfo.range = VK_WHOLE_SIZE;
                }
                else if (resource->IsBuffer())
                {
                    const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
                    auto internal_state = to_internal(buffer);
                    descriptor.bufferInfo.buffer = internal_state->resource;
                    descriptor.bufferInfo.offset = offset;
                    descriptor.bufferInfo.range = VK_WHOLE_SIZE;
                }
                else
                {
                    assert(0);
                }
                break;
            case RWTYPEDBUFFER:
                if (resource == nullptr || !resource->IsValid())
                {
                    descriptor.bufferView = nullBufferView;
                }
                else if (resource->IsBuffer())
                {
                    const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
                    auto internal_state = to_internal(buffer);
                    descriptor.bufferView = subresource < 0 ? internal_state->uav : internal_state->subresources_uav[subresource];
                }
                else
                {
                    assert(0);
                }
                break;
            case RWTEXTURE1D:
            case RWTEXTURE1DARRAY:
            case RWTEXTURE2D:
            case RWTEXTURE2DARRAY:
            case RWTEXTURE3D:
                descriptor.imageinfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                descriptor.imageinfo.sampler = VK_NULL_HANDLE;
                if (resource == nullptr || !resource->IsValid())
                {
                    switch (table->resources[rangeIndex].binding)
                    {
                        case TEXTURE1D:
                            descriptor.imageinfo.imageView = nullImageView1D;
                            break;
                        case TEXTURE1DARRAY:
                            descriptor.imageinfo.imageView = nullImageView1DArray;
                            break;
                        case TEXTURE2D:
                            descriptor.imageinfo.imageView = nullImageView2D;
                            break;
                        case TEXTURE2DARRAY:
                            descriptor.imageinfo.imageView = nullImageView2DArray;
                            break;
                        case TEXTURECUBE:
                            descriptor.imageinfo.imageView = nullImageViewCube;
                            break;
                        case TEXTURECUBEARRAY:
                            descriptor.imageinfo.imageView = nullImageViewCubeArray;
                            break;
                        case TEXTURE3D:
                            descriptor.imageinfo.imageView = nullImageView3D;
                            break;
                    };
                }
                else if (resource->IsTexture())
                {
                    const Texture* texture = (const Texture*) resource;
                    auto internal_state = to_internal(texture);
                    if (subresource < 0)
                    {
                        descriptor.imageinfo.imageView = internal_state->uav;
                    }
                    else
                    {
                        descriptor.imageinfo.imageView = internal_state->subresources_uav[subresource];
                    }
                }
                else
                {
                    assert(0);
                }
                break;
            default:
                break;
        }
    }
    void GraphicsDevice_Vulkan::WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler)
    {
        auto table_internal = to_internal(table);
        size_t sampler_remap = table->resources.size() + (size_t) rangeIndex;
        size_t remap = table_internal->sampler_write_remap[rangeIndex];
        auto& descriptor = table_internal->descriptors[remap];
        descriptor.imageinfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        descriptor.imageinfo.imageView = VK_NULL_HANDLE;

        if (sampler == nullptr)
        {
            descriptor.imageinfo.sampler = nullSampler;
        }
        else
        {
            auto internal_state = to_internal(sampler);
            descriptor.imageinfo.sampler = internal_state->resource;
        }
    }

    void GraphicsDevice_Vulkan::Map(const GPUResource* resource, Mapping* mapping)
    {
        VkDeviceMemory memory = VK_NULL_HANDLE;

        if (resource->type == GPUResource::GPU_RESOURCE_TYPE::BUFFER)
        {
            const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
            auto internal_state = to_internal(buffer);
            memory = internal_state->allocation->GetMemory();
            mapping->rowpitch = (uint32_t) buffer->GetDesc().ByteWidth;
        }
        else if (resource->type == GPUResource::GPU_RESOURCE_TYPE::TEXTURE)
        {
            const Texture* texture = (const Texture*) resource;
            auto internal_state = to_internal(texture);
            memory = internal_state->allocation->GetMemory();
            mapping->rowpitch = (uint32_t) internal_state->subresourcelayout.rowPitch;
        }
        else
        {
            assert(0);
            return;
        }

        VkDeviceSize offset = mapping->offset;
        VkDeviceSize size = mapping->size;

        VkResult res = vkMapMemory(device, memory, offset, size, 0, &mapping->data);
        if (res != VK_SUCCESS)
        {
            assert(0);
            mapping->data = nullptr;
            mapping->rowpitch = 0;
        }
    }
    void GraphicsDevice_Vulkan::Unmap(const GPUResource* resource)
    {
        if (resource->type == GPUResource::GPU_RESOURCE_TYPE::BUFFER)
        {
            const GraphicsBuffer* buffer = (const GraphicsBuffer*) resource;
            auto internal_state = to_internal(buffer);
            vkUnmapMemory(device, internal_state->allocation->GetMemory());
        }
        else if (resource->type == GPUResource::GPU_RESOURCE_TYPE::TEXTURE)
        {
            const Texture* texture = (const Texture*) resource;
            auto internal_state = to_internal(texture);
            vkUnmapMemory(device, internal_state->allocation->GetMemory());
        }
    }
    bool GraphicsDevice_Vulkan::QueryRead(const GPUQuery* query, GPUQueryResult* result)
    {
        auto internal_state = to_internal(query);

        VkResult res = VK_SUCCESS;

        switch (query->desc.Type)
        {
            case GPU_QUERY_TYPE_EVENT:
                assert(0); // not implemented yet
                break;
            case GPU_QUERY_TYPE_TIMESTAMP:
                res = vkGetQueryPoolResults(device, querypool_timestamp, (uint32_t) internal_state->query_index, 1, sizeof(uint64_t),
                                            &result->result_timestamp, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
                if (timestamps_to_reset.empty() || timestamps_to_reset.back() != (uint32_t) internal_state->query_index)
                {
                    timestamps_to_reset.push_back((uint32_t) internal_state->query_index);
                }
                break;
            case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
                result->result_timestamp_frequency = timestamp_frequency;
                break;
            case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
            case GPU_QUERY_TYPE_OCCLUSION:
                res = vkGetQueryPoolResults(device, querypool_occlusion, (uint32_t) internal_state->query_index, 1, sizeof(uint64_t),
                                            &result->result_passed_sample_count, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_PARTIAL_BIT);
                if (occlusions_to_reset.empty() || occlusions_to_reset.back() != (uint32_t) internal_state->query_index)
                {
                    occlusions_to_reset.push_back((uint32_t) internal_state->query_index);
                }
                break;
        }

        return res == VK_SUCCESS;
    }

    void GraphicsDevice_Vulkan::SetName(GPUResource* pResource, const char* name)
    {
        if (!debugUtils)
            return;

        VkDebugUtilsObjectNameInfoEXT nameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
        nameInfo.pObjectName = name;
        if (pResource->IsTexture())
        {
            nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
            nameInfo.objectHandle = (uint64_t) to_internal((const Texture*) pResource)->resource;
        }
        else if (pResource->IsBuffer())
        {
            nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
            nameInfo.objectHandle = (uint64_t) to_internal((const GraphicsBuffer*) pResource)->resource;
        }
        else if (pResource->IsAccelerationStructure())
        {
            nameInfo.objectType = VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
            nameInfo.objectHandle = (uint64_t) to_internal((const RaytracingAccelerationStructure*) pResource)->resource;
        }

        if (nameInfo.objectHandle == VK_NULL_HANDLE)
        {
            return;
        }

        VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &nameInfo));
    }

    VkSemaphore GraphicsDevice_Vulkan::RequestSemaphore()
    {
        VkSemaphore semaphore;
        if (recycledSemaphores.empty())
        {
            VkSemaphoreCreateInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            VK_CHECK(vkCreateSemaphore(device, &info, nullptr, &semaphore));
        }
        else
        {
            semaphore = recycledSemaphores.back();
            recycledSemaphores.pop_back();
        }

        return semaphore;
    }

    void GraphicsDevice_Vulkan::ReturnSemaphore(VkSemaphore semaphore)
    {
        recycledSemaphores.push_back(semaphore);
    }

    CommandList& GraphicsDevice_Vulkan::BeginCommandList()
    {
        std::atomic_uint32_t cmd = commandListsCount.fetch_add(1);
        ALIMER_ASSERT(cmd < kCommandListCount);

        if (commandLists[cmd] == nullptr)
        {
            QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

            commandLists[cmd] = new Vulkan_CommandList(this, cmd, queueFamilyIndices.graphicsFamily);
        }

        commandLists[cmd]->Reset(GetFrameIndex());

        VkCommandBuffer commandBuffer = commandLists[cmd]->GetDirectCommandList();

        VkViewport viewports[6];
        for (uint32_t i = 0; i < ALIMER_STATIC_ARRAY_SIZE(viewports); ++i)
        {
            viewports[i].x = 0;
            viewports[i].y = 0;
            viewports[i].width = (float) backbufferWidth;
            viewports[i].height = (float) backbufferHeight;
            viewports[i].minDepth = 0;
            viewports[i].maxDepth = 1;
        }
        vkCmdSetViewport(commandBuffer, 0, ALIMER_STATIC_ARRAY_SIZE(viewports), viewports);

        if (!initial_querypool_reset)
        {
            initial_querypool_reset = true;
            vkCmdResetQueryPool(commandBuffer, querypool_timestamp, 0, timestamp_query_count);
            vkCmdResetQueryPool(commandBuffer, querypool_occlusion, 0, occlusion_query_count);
        }
        for (auto& x : timestamps_to_reset)
        {
            vkCmdResetQueryPool(commandBuffer, querypool_timestamp, x, 1);
        }
        timestamps_to_reset.clear();
        for (auto& x : occlusions_to_reset)
        {
            vkCmdResetQueryPool(commandBuffer, querypool_occlusion, x, 1);
        }
        occlusions_to_reset.clear();

        return *commandLists[cmd];
    }

    void GraphicsDevice_Vulkan::SubmitCommandLists()
    {
        // Sync up copy queue and transitions:
        copyQueueLock.lock();
        if (copyQueueUse)
        {
            auto& frame = GetFrameResources();

            // Copies:
            {
                VkResult res = vkEndCommandBuffer(frame.copyCommandBuffer);
                assert(res == VK_SUCCESS);

                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &frame.copyCommandBuffer;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &copySemaphore;

                res = vkQueueSubmit(frame.copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
                assert(res == VK_SUCCESS);
            }

            // Transitions:
            {
                for (auto& barrier : frame.loadedimagetransitions)
                {
                    vkCmdPipelineBarrier(
                        frame.transitionCommandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);
                }
                frame.loadedimagetransitions.clear();

                VkResult res = vkEndCommandBuffer(frame.transitionCommandBuffer);
                assert(res == VK_SUCCESS);

                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &frame.transitionCommandBuffer;

                res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
                assert(res == VK_SUCCESS);
            }
        }

        // Execute deferred command lists:
        {
            auto& frame = GetFrameResources();

            // Submit it to the queue with a release semaphore.
            if (frame.swapchainReleaseSemaphore == VK_NULL_HANDLE)
            {
                VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
                VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.swapchainReleaseSemaphore));
            }

            VkCommandBuffer commandBuffers[kCommandListCount];
            uint32_t counter = 0;

            uint32_t cmd_last = commandListsCount.load();
            commandListsCount.store(0);
            for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
            {
                commandBuffers[counter] = commandLists[counter]->End();
                counter++;
            }

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {frame.swapchainAcquireSemaphore, copySemaphore};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
            if (copyQueueUse)
            {
                submitInfo.waitSemaphoreCount = 2;
            }
            else
            {
                submitInfo.waitSemaphoreCount = 1;
            }
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = counter;
            submitInfo.pCommandBuffers = commandBuffers;

            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &frame.swapchainReleaseSemaphore;

            VkResult res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.frameFence);
            assert(res == VK_SUCCESS);
        }

        // This acts as a barrier, following this we will be using the next frame's resources when calling GetFrameResources()!
        FRAMECOUNT++;
        frameIndex = FRAMECOUNT % BACKBUFFER_COUNT;

        // Initiate stalling CPU when GPU is behind by more frames than would fit in the backbuffers:
        if (FRAMECOUNT >= BACKBUFFER_COUNT)
        {
            VkResult res = vkWaitForFences(device, 1, &GetFrameResources().frameFence, true, UINT64_MAX);
            assert(res == VK_SUCCESS);

            res = vkResetFences(device, 1, &GetFrameResources().frameFence);
            assert(res == VK_SUCCESS);
        }

        allocationhandler->Update(FRAMECOUNT, BACKBUFFER_COUNT);

        // Restart transition command buffers:
        {
            auto& frame = GetFrameResources();

            VkResult res = vkResetCommandPool(device, frame.transitionCommandPool, 0);
            assert(res == VK_SUCCESS);

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr; // Optional

            res = vkBeginCommandBuffer(frame.transitionCommandBuffer, &beginInfo);
            assert(res == VK_SUCCESS);
        }

        copyQueueUse = false;
        copyQueueLock.unlock();
    }

    void GraphicsDevice_Vulkan::WaitForGPU()
    {
        VkResult res = vkQueueWaitIdle(graphicsQueue);
        assert(res == VK_SUCCESS);
    }

    void GraphicsDevice_Vulkan::ClearPipelineStateCache()
    {
        allocationhandler->destroylocker.lock();
        for (auto& x : pipelines_global)
        {
            allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
        }
        pipelines_global.clear();

        /*for (int i = 0; i < ALIMER_STATIC_ARRAY_SIZE(pipelines_worker); ++i)
        {
            for (auto& x : pipelines_worker[i])
            {
                allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
            }
            pipelines_worker[i].clear();
        }*/
        allocationhandler->destroylocker.unlock();
    }

    /* Vulkan_CommandList */
    Vulkan_CommandList::Vulkan_CommandList(GraphicsDevice_Vulkan* device_, uint32_t index_, uint32_t queueFamilyIndex)
        : device(device_)
        , index(index_)
    {
        VkResult res;

        for (uint32_t i = 0; i < ALIMER_STATIC_ARRAY_SIZE(commandPools); i++)
        {
            VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            poolInfo.queueFamilyIndex = queueFamilyIndex;

            res = vkCreateCommandPool(device->GetVkDevice(), &poolInfo, nullptr, &commandPools[i]);
            assert(res == VK_SUCCESS);

            VkCommandBufferAllocateInfo commandBufferInfo = {};
            commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferInfo.commandBufferCount = 1;
            commandBufferInfo.commandPool = commandPools[i];
            commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            res = vkAllocateCommandBuffers(device->GetVkDevice(), &commandBufferInfo, &commandBuffers[i]);
            assert(res == VK_SUCCESS);

            resourceBuffer[i].init(device_, 1024 * 1024); // 1 MB starting size
            descriptors[i].init(device_);
        }
    }

    Vulkan_CommandList::~Vulkan_CommandList()
    {
        for (auto& pair : pipelines_worker)
        {
            vkDestroyPipeline(device->GetVkDevice(), pair.second, nullptr);
        }

        for (uint32_t i = 0; i < ALIMER_STATIC_ARRAY_SIZE(descriptors); i++)
        {
            descriptors[i].destroy();
        }

        for (uint32_t i = 0; i < ALIMER_STATIC_ARRAY_SIZE(commandPools); i++)
        {
            vkDestroyCommandPool(device->GetVkDevice(), commandPools[i], nullptr);
        }
    }

    void Vulkan_CommandList::Reset(uint32_t frameIndex_)
    {
        frameIndex = frameIndex_;

        VK_CHECK(vkResetCommandPool(device->GetVkDevice(), commandPools[frameIndex], 0));

        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        VK_CHECK(vkBeginCommandBuffer(commandBuffers[frameIndex], &beginInfo));

        VkRect2D scissors[8];
        for (int i = 0; i < ALIMER_STATIC_ARRAY_SIZE(scissors); ++i)
        {
            scissors[i].offset.x = 0;
            scissors[i].offset.y = 0;
            scissors[i].extent.width = 65535;
            scissors[i].extent.height = 65535;
        }
        vkCmdSetScissor(commandBuffers[frameIndex], 0, ALIMER_STATIC_ARRAY_SIZE(scissors), scissors);

        // TODO: @see V-EZ set also line width, depth bias, depth bounds

        float blendConstants[] = {1, 1, 1, 1};
        vkCmdSetBlendConstants(commandBuffers[frameIndex], blendConstants);

        // reset descriptor allocators:
        descriptors[frameIndex].reset();

        // reset immediate resource allocators:
        resourceBuffer[frameIndex].clear();

        active_renderpass = nullptr;
        prev_pipeline_hash = 0;
        active_pso = nullptr;
        active_cs = nullptr;
        active_rt = nullptr;
        dirty_pso = false;
    }

    VkCommandBuffer Vulkan_CommandList::End()
    {
        VK_CHECK(vkEndCommandBuffer(GetDirectCommandList()));

        for (auto& x : pipelines_worker)
        {
            if (device->pipelines_global.count(x.first) == 0)
            {
                device->pipelines_global[x.first] = x.second;
            }
            else
            {
                device->allocationhandler->destroylocker.lock();
                device->allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, device->GetFrameCount()));
                device->allocationhandler->destroylocker.unlock();
            }
        }
        pipelines_worker.clear();

        return GetDirectCommandList();
    }

    void Vulkan_CommandList::PresentBegin()
    {
        VkSemaphore acquireSemaphore = device->RequestSemaphore();

        VkResult res = vkAcquireNextImageKHR(
            device->GetVkDevice(),
            device->swapChain,
            UINT64_MAX,
            acquireSemaphore,
            VK_NULL_HANDLE,
            &device->swapChainImageIndex);

        if (res != VK_SUCCESS)
        {
            // Handle outdated error in acquire.
            if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
            {
                device->CreateBackBufferResources();
                PresentBegin();
                device->ReturnSemaphore(acquireSemaphore);
                return;
            }
        }

        assert(res == VK_SUCCESS);

        // Recycle the old semaphore back into the semaphore manager.
        VkSemaphore oldAcquireSemaphore = device->GetFrameResources().swapchainAcquireSemaphore;

        if (oldAcquireSemaphore != VK_NULL_HANDLE)
        {
            device->ReturnSemaphore(oldAcquireSemaphore);
        }

        device->GetFrameResources().swapchainAcquireSemaphore = acquireSemaphore;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassInfo.renderPass = device->defaultRenderPass;
        renderPassInfo.framebuffer = device->swapChainFramebuffers[device->swapChainImageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = device->swapChainExtent;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(GetDirectCommandList(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void Vulkan_CommandList::PresentEnd()
    {
        vkCmdEndRenderPass(GetDirectCommandList());
        device->SubmitCommandLists();

        VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &device->frames[device->swapChainImageIndex].swapchainReleaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &device->swapChain;
        presentInfo.pImageIndices = &device->swapChainImageIndex;

        VkResult result = vkQueuePresentKHR(device->presentQueue, &presentInfo);
        // Handle Outdated error in present.
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            device->CreateBackBufferResources();
        }
        else if (result != VK_SUCCESS)
        {
            LOGE("Failed to present swapchain image.");
        }
    }

    void Vulkan_CommandList::PushDebugGroup(const char* name)
    {
        if (!device->debugUtils)
            return;

        VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name;
        label.color[0] = 0;
        label.color[1] = 0;
        label.color[2] = 0;
        label.color[3] = 1;
        vkCmdBeginDebugUtilsLabelEXT(GetDirectCommandList(), &label);
    }

    void Vulkan_CommandList::PopDebugGroup()
    {
        if (!device->debugUtils)
            return;

        vkCmdEndDebugUtilsLabelEXT(GetDirectCommandList());
    }

    void Vulkan_CommandList::InsertDebugMarker(const char* name)
    {
        if (!device->debugUtils)
            return;

        VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name;
        label.color[0] = 0;
        label.color[1] = 0;
        label.color[2] = 0;
        label.color[3] = 1;
        vkCmdInsertDebugUtilsLabelEXT(GetDirectCommandList(), &label);
    }

    void Vulkan_CommandList::RenderPassBegin(const RenderPass* renderpass)
    {
        active_renderpass = renderpass;

        auto internal_state = to_internal(renderpass);
        vkCmdBeginRenderPass(GetDirectCommandList(), &internal_state->beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void Vulkan_CommandList::RenderPassEnd()
    {
        vkCmdEndRenderPass(GetDirectCommandList());
        active_renderpass = nullptr;
    }

    void Vulkan_CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        viewports[0].x = x;
        viewports[0].y = y;
        viewports[0].width = width;
        viewports[0].height = height;
        viewports[0].minDepth = minDepth;
        viewports[0].maxDepth = maxDepth;
        vkCmdSetViewport(GetDirectCommandList(), 0, 1, &viewports[0]);
    }

    void Vulkan_CommandList::SetViewport(const Viewport& viewport)
    {
        viewports[0].x = viewport.x;
        viewports[0].y = viewport.y;
        viewports[0].width = viewport.width;
        viewports[0].height = viewport.height;
        viewports[0].minDepth = viewport.minDepth;
        viewports[0].maxDepth = viewport.maxDepth;
        vkCmdSetViewport(GetDirectCommandList(), 0, 1, &viewports[0]);
    }

    void Vulkan_CommandList::SetViewports(uint32_t viewportCount, const Viewport* pViewports)
    {
        ALIMER_ASSERT(viewportCount <= kMaxViewportAndScissorRects);

        for (uint32_t i = 0; i < viewportCount; ++i)
        {
            viewports[i].x = pViewports[i].x;
            viewports[i].y = pViewports[i].y;
            viewports[i].width = pViewports[i].width;
            viewports[i].height = pViewports[i].height;
            viewports[i].minDepth = pViewports[i].minDepth;
            viewports[i].maxDepth = pViewports[i].maxDepth;
        }

        vkCmdSetViewport(GetDirectCommandList(), 0, viewportCount, viewports);
    }

    void Vulkan_CommandList::SetScissorRect(const ScissorRect& rect)
    {
        scissors[0].offset.x = rect.x;
        scissors[0].offset.y = rect.y;
        scissors[0].extent.width = uint32_t(rect.width);
        scissors[0].extent.height = uint32_t(rect.height);
        vkCmdSetScissor(GetDirectCommandList(), 0, 1, &scissors[0]);
    }

    void Vulkan_CommandList::SetScissorRects(uint32_t scissorCount, const ScissorRect* rects)
    {
        ALIMER_ASSERT(rects != nullptr);
        ALIMER_ASSERT(scissorCount <= kMaxViewportAndScissorRects);

        for (uint32_t i = 0; i < scissorCount; ++i)
        {
            scissors[i].offset.x = int32_t(rects[i].x);
            scissors[i].offset.y = int32_t(rects[i].y);
            scissors[i].extent.width = uint32_t(rects[i].width);
            scissors[i].extent.height = uint32_t(rects[i].height);
        }

        vkCmdSetScissor(GetDirectCommandList(), 0, scissorCount, scissors);
    }

    void Vulkan_CommandList::BindResource(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource)
    {
        ALIMER_ASSERT(slot < GPU_RESOURCE_HEAP_SRV_COUNT);
        if (descriptors[frameIndex].SRV[slot] != resource || descriptors[frameIndex].SRV_index[slot] != subresource)
        {
            descriptors[frameIndex].SRV[slot] = resource;
            descriptors[frameIndex].SRV_index[slot] = subresource;
            descriptors[frameIndex].dirty = true;
        }
    }

    void Vulkan_CommandList::BindResources(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count)
    {
        if (resources != nullptr)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                BindResource(stage, resources[i], slot + i, -1);
            }
        }
    }

    void Vulkan_CommandList::BindUAV(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource)
    {
        ALIMER_ASSERT(slot < GPU_RESOURCE_HEAP_UAV_COUNT);
        if (descriptors[frameIndex].UAV[slot] != resource || descriptors[frameIndex].UAV_index[slot] != subresource)
        {
            descriptors[frameIndex].UAV[slot] = resource;
            descriptors[frameIndex].UAV_index[slot] = subresource;
            descriptors[frameIndex].dirty = true;
        }
    }

    void Vulkan_CommandList::BindUAVs(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count)
    {
        if (resources != nullptr)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                BindUAV(stage, resources[i], slot + i, -1);
            }
        }
    }

    void Vulkan_CommandList::BindSampler(ShaderStage stage, const Sampler* sampler, uint32_t slot)
    {
        ALIMER_ASSERT(slot < GPU_SAMPLER_HEAP_COUNT);
        if (descriptors[frameIndex].SAM[slot] != sampler)
        {
            descriptors[frameIndex].SAM[slot] = sampler;
            descriptors[frameIndex].dirty = true;
        }
    }

    void Vulkan_CommandList::BindConstantBuffer(ShaderStage stage, const GraphicsBuffer* buffer, uint32_t slot)
    {
        ALIMER_ASSERT(slot < GPU_RESOURCE_HEAP_CBV_COUNT);
        if (buffer->GetDesc().Usage == USAGE_DYNAMIC || descriptors[frameIndex].CBV[slot] != buffer)
        {
            descriptors[frameIndex].CBV[slot] = buffer;
            descriptors[frameIndex].dirty = true;
        }
    }

    void Vulkan_CommandList::BindVertexBuffers(const GraphicsBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets)
    {
        VkDeviceSize voffsets[8] = {};
        VkBuffer vbuffers[8] = {};
        assert(count <= 8);
        for (uint32_t i = 0; i < count; ++i)
        {
            if (vertexBuffers[i] == nullptr)
            {
                vbuffers[i] = device->nullBuffer;
            }
            else
            {
                auto internal_state = to_internal(vertexBuffers[i]);
                vbuffers[i] = internal_state->resource;
                if (offsets != nullptr)
                {
                    voffsets[i] = (VkDeviceSize) offsets[i];
                }
            }
        }

        vkCmdBindVertexBuffers(GetDirectCommandList(), slot, count, vbuffers, voffsets);
    }

    void Vulkan_CommandList::BindIndexBuffer(const GraphicsBuffer* indexBuffer, IndexFormat format, uint32_t offset)
    {
        if (indexBuffer != nullptr)
        {
            auto internal_state = to_internal(indexBuffer);
            vkCmdBindIndexBuffer(GetDirectCommandList(), internal_state->resource, (VkDeviceSize) offset, format == IndexFormat::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        }
    }

    void Vulkan_CommandList::BindStencilRef(uint32_t value)
    {
        vkCmdSetStencilReference(GetDirectCommandList(), VK_STENCIL_FRONT_AND_BACK, value);
    }

    void Vulkan_CommandList::BindBlendFactor(float r, float g, float b, float a)
    {
        float blendConstants[] = {r, g, b, a};
        vkCmdSetBlendConstants(GetDirectCommandList(), blendConstants);
    }

    void Vulkan_CommandList::SetRenderPipeline(RenderPipeline* pipeline)
    {
        size_t pipeline_hash = 0;
        alimer::CombineHash(pipeline_hash, to_internal(pipeline)->hash);
        if (active_renderpass != nullptr)
        {
            alimer::CombineHash(pipeline_hash, active_renderpass->hash);
        }

        if (prev_pipeline_hash == pipeline_hash)
        {
            return;
        }
        prev_pipeline_hash = pipeline_hash;

        descriptors[frameIndex].dirty = true;
        active_pso = pipeline;
        dirty_pso = true;
    }

    void Vulkan_CommandList::BindComputeShader(const Shader* shader)
    {
        ALIMER_ASSERT(shader->stage == ShaderStage::Compute);

        if (active_cs != shader)
        {
            descriptors[frameIndex].dirty = true;
            active_cs = shader;
            auto internal_state = to_internal(shader);
            vkCmdBindPipeline(GetDirectCommandList(), VK_PIPELINE_BIND_POINT_COMPUTE, internal_state->pipeline_cs);
        }
    }

    void Vulkan_CommandList::FlushPipeline()
    {
        if (!dirty_pso)
            return;

        auto pso = to_internal(active_pso);
        size_t pipeline_hash = prev_pipeline_hash;

        VkPipeline pipeline = VK_NULL_HANDLE;
        auto it = device->pipelines_global.find(pipeline_hash);
        if (it == device->pipelines_global.end())
        {
            for (auto& x : pipelines_worker)
            {
                if (pipeline_hash == x.first)
                {
                    pipeline = x.second;
                    break;
                }
            }

            if (pipeline == VK_NULL_HANDLE)
            {
                VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
                //pipelineInfo.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
                if (pso->desc.rootSignature == nullptr)
                {
                    pipelineInfo.layout = to_internal(pso)->pipelineLayout;
                }
                else
                {
                    pipelineInfo.layout = to_internal(pso->desc.rootSignature)->pipelineLayout;
                }
                pipelineInfo.renderPass = active_renderpass == nullptr ? device->defaultRenderPass : to_internal(active_renderpass)->renderpass;
                pipelineInfo.subpass = 0;
                pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

                // Shaders:

                uint32_t shaderStageCount = 0;
                VkPipelineShaderStageCreateInfo shaderStages[static_cast<uint32_t>(ShaderStage::Count) - 1];
                if (pso->desc.ms != nullptr && pso->desc.ms->IsValid())
                {
                    shaderStages[shaderStageCount++] = to_internal(pso->desc.ms)->stageInfo;
                }
                if (pso->desc.as != nullptr && pso->desc.as->IsValid())
                {
                    shaderStages[shaderStageCount++] = to_internal(pso->desc.as)->stageInfo;
                }
                if (pso->desc.vs != nullptr && pso->desc.vs->IsValid())
                {
                    shaderStages[shaderStageCount++] = to_internal(pso->desc.vs)->stageInfo;
                }
                if (pso->desc.hs != nullptr && pso->desc.hs->IsValid())
                {
                    shaderStages[shaderStageCount++] = to_internal(pso->desc.hs)->stageInfo;
                }
                if (pso->desc.ds != nullptr && pso->desc.ds->IsValid())
                {
                    shaderStages[shaderStageCount++] = to_internal(pso->desc.ds)->stageInfo;
                }
                if (pso->desc.gs != nullptr && pso->desc.gs->IsValid())
                {
                    shaderStages[shaderStageCount++] = to_internal(pso->desc.gs)->stageInfo;
                }
                if (pso->desc.ps != nullptr && pso->desc.ps->IsValid())
                {
                    shaderStages[shaderStageCount++] = to_internal(pso->desc.ps)->stageInfo;
                }
                pipelineInfo.stageCount = shaderStageCount;
                pipelineInfo.pStages = shaderStages;

                // Fixed function states
                // Input layout:
                VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

                uint32_t vertexBindingDescriptionCount = 0;
                uint32_t vertexAttributeDescriptionCount = 0;
                std::array<VkVertexInputBindingDescription, kMaxVertexBufferBindings> vertexBindingDescriptions;
                std::array<VkVertexInputAttributeDescription, kMaxVertexAttributes> vertexAttributeDescriptions;

                // Layout first
                for (uint32_t binding = 0; binding < kMaxVertexBufferBindings; ++binding)
                {
                    const VertexBufferLayoutDescriptor* layoutDesc = &pso->desc.vertexDescriptor.layouts[binding];
                    if (layoutDesc->stride == 0)
                    {
                        break;
                    }

                    VkVertexInputBindingDescription* vkVertexBindingDescription = &vertexBindingDescriptions[vertexBindingDescriptionCount++];
                    vkVertexBindingDescription->binding = binding;
                    vkVertexBindingDescription->stride = layoutDesc->stride;
                    vkVertexBindingDescription->inputRate = layoutDesc->stepMode == InputStepMode::Vertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
                }

                for (uint32_t location = 0; location < kMaxVertexAttributes; ++location)
                {
                    const VertexAttributeDescriptor* attrDesc = &pso->desc.vertexDescriptor.attributes[location];
                    if (attrDesc->format == VertexFormat::Invalid)
                    {
                        break;
                    }

                    VkVertexInputAttributeDescription* vkVertexAttributeDesc = &vertexAttributeDescriptions[vertexAttributeDescriptionCount++];
                    vkVertexAttributeDesc->location = location;
                    vkVertexAttributeDesc->binding = attrDesc->bufferIndex;
                    vkVertexAttributeDesc->format = _ConvertVertexFormat(attrDesc->format);
                    vkVertexAttributeDesc->offset = attrDesc->offset;
                }

                vertexInputInfo.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
                vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
                vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
                vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
                pipelineInfo.pVertexInputState = &vertexInputInfo;

                // Primitive type:
                VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                switch (pso->desc.primitiveTopology)
                {
                    case PrimitiveTopology::PointList:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                        break;
                    case PrimitiveTopology::LineList:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                        break;
                    case PrimitiveTopology::LineStrip:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                        break;
                    case PrimitiveTopology::TriangleList:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                        break;
                    case PrimitiveTopology::TriangleStrip:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                        break;
                    case PrimitiveTopology::PatchList:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
                        break;
                    default:
                        break;
                }
                inputAssembly.primitiveRestartEnable = VK_FALSE;

                pipelineInfo.pInputAssemblyState = &inputAssembly;

                // Rasterization State:
                const RasterizationStateDescriptor& rasterizationState = pso->desc.rasterizationState;
                // depth clip will be enabled via Vulkan 1.1 extension VK_EXT_depth_clip_enable:
                VkPipelineRasterizationDepthClipStateCreateInfoEXT depthclip = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT};
                depthclip.depthClipEnable = VK_TRUE;

                VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
                rasterizer.pNext = &depthclip;
                rasterizer.depthClampEnable = VK_TRUE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
                switch (rasterizationState.cullMode)
                {
                    case CullMode::Back:
                        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                        break;
                    case CullMode::Front:
                        rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
                        break;
                    case CullMode::None:
                    default:
                        rasterizer.cullMode = VK_CULL_MODE_NONE;
                        break;
                }

                rasterizer.frontFace = (rasterizationState.frontFace == FrontFace::CCW) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
                rasterizer.depthBiasEnable = rasterizationState.depthBias != 0 || rasterizationState.depthBiasSlopeScale != 0;
                rasterizer.depthBiasConstantFactor = static_cast<float>(rasterizationState.depthBias);
                rasterizer.depthBiasClamp = rasterizationState.depthBiasClamp;
                rasterizer.depthBiasSlopeFactor = rasterizationState.depthBiasSlopeScale;
                rasterizer.lineWidth = 1.0f;

                // depth clip is extension in Vulkan 1.1:
                depthclip.depthClipEnable = rasterizationState.depthClipEnable ? VK_TRUE : VK_FALSE;
                pipelineInfo.pRasterizationState = &rasterizer;

                // Viewport, Scissor:
                VkViewport viewport = {};
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = 65535;
                viewport.height = 65535;
                viewport.minDepth = 0;
                viewport.maxDepth = 1;

                VkRect2D scissor = {};
                scissor.extent.width = 65535;
                scissor.extent.height = 65535;

                VkPipelineViewportStateCreateInfo viewportState = {};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.pViewports = &viewport;
                viewportState.scissorCount = 1;
                viewportState.pScissors = &scissor;

                pipelineInfo.pViewportState = &viewportState;

                // Depth-Stencil:
                VkPipelineDepthStencilStateCreateInfo depthstencil = {};
                depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthstencil.depthTestEnable = (pso->desc.depthStencilState.depthCompare != CompareFunction::Always || pso->desc.depthStencilState.depthWriteEnabled) ? VK_TRUE : VK_FALSE;
                depthstencil.depthWriteEnable = pso->desc.depthStencilState.depthWriteEnabled ? VK_FALSE : VK_TRUE;
                depthstencil.depthCompareOp = _ConvertComparisonFunc(pso->desc.depthStencilState.depthCompare);

                depthstencil.stencilTestEnable = StencilTestEnabled(&pso->desc.depthStencilState) ? VK_TRUE : VK_FALSE;

                depthstencil.front.compareMask = pso->desc.depthStencilState.stencilReadMask;
                depthstencil.front.writeMask = pso->desc.depthStencilState.stencilWriteMask;
                depthstencil.front.reference = 0; // runtime supplied
                depthstencil.front.compareOp = _ConvertComparisonFunc(pso->desc.depthStencilState.stencilFront.compare);
                depthstencil.front.passOp = _ConvertStencilOp(pso->desc.depthStencilState.stencilFront.passOp);
                depthstencil.front.failOp = _ConvertStencilOp(pso->desc.depthStencilState.stencilFront.failOp);
                depthstencil.front.depthFailOp = _ConvertStencilOp(pso->desc.depthStencilState.stencilFront.depthFailOp);

                depthstencil.back.compareMask = pso->desc.depthStencilState.stencilReadMask;
                depthstencil.back.writeMask = pso->desc.depthStencilState.stencilWriteMask;
                depthstencil.back.reference = 0; // runtime supplied
                depthstencil.back.compareOp = _ConvertComparisonFunc(pso->desc.depthStencilState.stencilBack.compare);
                depthstencil.back.passOp = _ConvertStencilOp(pso->desc.depthStencilState.stencilBack.passOp);
                depthstencil.back.failOp = _ConvertStencilOp(pso->desc.depthStencilState.stencilBack.failOp);
                depthstencil.back.depthFailOp = _ConvertStencilOp(pso->desc.depthStencilState.stencilBack.depthFailOp);

                depthstencil.depthBoundsTestEnable = VK_FALSE;
                pipelineInfo.pDepthStencilState = &depthstencil;

                // MSAA:
                VkPipelineMultisampleStateCreateInfo multisampling = {};
                multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable = VK_FALSE;
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                if (active_renderpass != nullptr && active_renderpass->desc.attachments.size() > 0)
                {
                    multisampling.rasterizationSamples = (VkSampleCountFlagBits) active_renderpass->desc.attachments[0].texture->desc.SampleCount;
                }
                multisampling.minSampleShading = 1.0f;
                VkSampleMask samplemask = pso->desc.sampleMask;
                multisampling.pSampleMask = &samplemask;
                multisampling.alphaToCoverageEnable = VK_FALSE;
                multisampling.alphaToOneEnable = VK_FALSE;

                pipelineInfo.pMultisampleState = &multisampling;

                // Blending:
                uint32_t numBlendAttachments = 0;
                VkPipelineColorBlendAttachmentState colorBlendAttachments[8];
                const size_t blend_loopCount = active_renderpass == nullptr ? 1u : active_renderpass->desc.attachments.size();
                for (size_t i = 0; i < blend_loopCount; ++i)
                {
                    if (active_renderpass != nullptr && active_renderpass->desc.attachments[i].type != RenderPassAttachment::RENDERTARGET)
                    {
                        continue;
                    }

                    ColorAttachmentDescriptor desc = pso->desc.colorAttachments[numBlendAttachments];
                    VkPipelineColorBlendAttachmentState& attachment = colorBlendAttachments[numBlendAttachments];
                    numBlendAttachments++;

                    attachment.blendEnable = desc.blendEnable ? VK_TRUE : VK_FALSE;

                    attachment.colorWriteMask = 0;
                    if (any(desc.colorWriteMask & ColorWriteMask::Red))
                    {
                        attachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
                    }
                    if (any(desc.colorWriteMask & ColorWriteMask::Green))
                    {
                        attachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
                    }
                    if (any(desc.colorWriteMask & ColorWriteMask::Blue))
                    {
                        attachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
                    }
                    if (any(desc.colorWriteMask & ColorWriteMask::Alpha))
                    {
                        attachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
                    }

                    attachment.srcColorBlendFactor = _ConvertBlend(desc.srcColorBlendFactor);
                    attachment.dstColorBlendFactor = _ConvertBlend(desc.dstColorBlendFactor);
                    attachment.colorBlendOp = _ConvertBlendOp(desc.colorBlendOp);
                    attachment.srcAlphaBlendFactor = _ConvertBlend(desc.srcAlphaBlendFactor);
                    attachment.dstAlphaBlendFactor = _ConvertBlend(desc.dstAlphaBlendFactor);
                    attachment.alphaBlendOp = _ConvertBlendOp(desc.alphaBlendOp);
                }

                VkPipelineColorBlendStateCreateInfo colorBlending = {};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;
                colorBlending.logicOp = VK_LOGIC_OP_COPY;
                colorBlending.attachmentCount = numBlendAttachments;
                colorBlending.pAttachments = colorBlendAttachments;
                colorBlending.blendConstants[0] = 1.0f;
                colorBlending.blendConstants[1] = 1.0f;
                colorBlending.blendConstants[2] = 1.0f;
                colorBlending.blendConstants[3] = 1.0f;

                pipelineInfo.pColorBlendState = &colorBlending;

                // Tessellation:
                VkPipelineTessellationStateCreateInfo tessellationInfo = {};
                tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
                tessellationInfo.patchControlPoints = 3;

                pipelineInfo.pTessellationState = &tessellationInfo;

                // Dynamic state will be specified at runtime:
                VkDynamicState dynamicStates[] = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR,
                    VK_DYNAMIC_STATE_STENCIL_REFERENCE,
                    VK_DYNAMIC_STATE_BLEND_CONSTANTS};

                VkPipelineDynamicStateCreateInfo dynamicState = {};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.dynamicStateCount = ALIMER_STATIC_ARRAY_SIZE(dynamicStates);
                dynamicState.pDynamicStates = dynamicStates;

                pipelineInfo.pDynamicState = &dynamicState;

                VkResult res = vkCreateGraphicsPipelines(device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
                assert(res == VK_SUCCESS);

                pipelines_worker.push_back(std::make_pair(pipeline_hash, pipeline));
            }
        }
        else
        {
            pipeline = it->second;
        }
        assert(pipeline != VK_NULL_HANDLE);

        vkCmdBindPipeline(GetDirectCommandList(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    void Vulkan_CommandList::PrepareDraw()
    {
        FlushPipeline();

        auto internal_state_pso = to_internal(active_pso);

        if (internal_state_pso->desc.rootSignature == nullptr)
        {
            descriptors[frameIndex].validate(true, this);
        }
        else
        {
            auto rootsig_internal = to_internal(internal_state_pso->desc.rootSignature);
            if (rootsig_internal->dirty[index])
            {
                rootsig_internal->dirty[index] = false;
                vkCmdBindDescriptorSets(
                    GetDirectCommandList(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    rootsig_internal->pipelineLayout,
                    0,
                    (uint32_t) rootsig_internal->last_descriptorsets[index].size(),
                    rootsig_internal->last_descriptorsets[index].data(),
                    (uint32_t) rootsig_internal->root_offsets[index].size(),
                    rootsig_internal->root_offsets[index].data());
            }
        }
    }

    void Vulkan_CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        PrepareDraw();
        vkCmdDraw(GetDirectCommandList(), vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void Vulkan_CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
    {
        PrepareDraw();
        vkCmdDrawIndexed(GetDirectCommandList(), indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }

    void Vulkan_CommandList::DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
        PrepareDraw();
        auto internal_state = to_internal(args);
        vkCmdDrawIndirect(GetDirectCommandList(), internal_state->resource, (VkDeviceSize) args_offset, 1, (uint32_t) sizeof(IndirectDrawArgsInstanced));
    }

    void Vulkan_CommandList::DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
        PrepareDraw();
        auto internal_state = to_internal(args);
        vkCmdDrawIndexedIndirect(GetDirectCommandList(), internal_state->resource, (VkDeviceSize) args_offset, 1, (uint32_t) sizeof(IndirectDrawArgsIndexedInstanced));
    }

    void Vulkan_CommandList::PrepareDispatch()
    {
        if (active_cs->rootSignature == nullptr)
        {
            descriptors[frameIndex].validate(false, this);
        }
        else
        {
            auto rootsig_internal = to_internal(active_cs->rootSignature);
            if (rootsig_internal->dirty[index])
            {
                rootsig_internal->dirty[index] = false;
                vkCmdBindDescriptorSets(
                    GetDirectCommandList(),
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    rootsig_internal->pipelineLayout,
                    0,
                    (uint32_t) rootsig_internal->last_descriptorsets[index].size(),
                    rootsig_internal->last_descriptorsets[index].data(),
                    (uint32_t) rootsig_internal->root_offsets[index].size(),
                    rootsig_internal->root_offsets[index].data());
            }
        }
    }

    void Vulkan_CommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        PrepareDispatch();
        vkCmdDispatch(GetDirectCommandList(), groupCountX, groupCountY, groupCountZ);
    }

    void Vulkan_CommandList::DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
    }

    void Vulkan_CommandList::CopyResource(const GPUResource* pDst, const GPUResource* pSrc)
    {
        if (pDst->type == GPUResource::GPU_RESOURCE_TYPE::TEXTURE && pSrc->type == GPUResource::GPU_RESOURCE_TYPE::TEXTURE)
        {
            auto internal_state_src = to_internal((const Texture*) pSrc);
            auto internal_state_dst = to_internal((const Texture*) pDst);

            const TextureDesc& src_desc = ((const Texture*) pSrc)->GetDesc();
            const TextureDesc& dst_desc = ((const Texture*) pDst)->GetDesc();

            if (src_desc.Usage & USAGE_STAGING)
            {
                VkBufferImageCopy copy = {};
                copy.imageExtent.width = dst_desc.Width;
                copy.imageExtent.height = dst_desc.Height;
                copy.imageExtent.depth = 1;
                copy.imageExtent.width = dst_desc.Width;
                copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copy.imageSubresource.layerCount = 1;
                vkCmdCopyBufferToImage(
                    GetDirectCommandList(),
                    internal_state_src->staging_resource,
                    internal_state_dst->resource,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copy);
            }
            else if (dst_desc.Usage & USAGE_STAGING)
            {
                VkBufferImageCopy copy = {};
                copy.imageExtent.width = src_desc.Width;
                copy.imageExtent.height = src_desc.Height;
                copy.imageExtent.depth = 1;
                copy.imageExtent.width = src_desc.Width;
                copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copy.imageSubresource.layerCount = 1;
                vkCmdCopyImageToBuffer(
                    GetDirectCommandList(),
                    internal_state_src->resource,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    internal_state_dst->staging_resource,
                    1,
                    &copy);
            }
            else
            {
                VkImageCopy copy = {};
                copy.extent.width = dst_desc.Width;
                copy.extent.height = dst_desc.Height;
                copy.extent.depth = std::max(1u, dst_desc.Depth);

                copy.srcOffset.x = 0;
                copy.srcOffset.y = 0;
                copy.srcOffset.z = 0;

                copy.dstOffset.x = 0;
                copy.dstOffset.y = 0;
                copy.dstOffset.z = 0;

                if (src_desc.BindFlags & BIND_DEPTH_STENCIL)
                {
                    copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    if (device->IsFormatStencilSupport(src_desc.format))
                    {
                        copy.srcSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                    }
                }
                else
                {
                    copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                }
                copy.srcSubresource.baseArrayLayer = 0;
                copy.srcSubresource.layerCount = src_desc.ArraySize;
                copy.srcSubresource.mipLevel = 0;

                if (dst_desc.BindFlags & BIND_DEPTH_STENCIL)
                {
                    copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    if (device->IsFormatStencilSupport(dst_desc.format))
                    {
                        copy.dstSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                    }
                }
                else
                {
                    copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                }
                copy.dstSubresource.baseArrayLayer = 0;
                copy.dstSubresource.layerCount = dst_desc.ArraySize;
                copy.dstSubresource.mipLevel = 0;

                vkCmdCopyImage(GetDirectCommandList(),
                               internal_state_src->resource, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               internal_state_dst->resource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &copy);
            }
        }
        else if (pDst->type == GPUResource::GPU_RESOURCE_TYPE::BUFFER && pSrc->type == GPUResource::GPU_RESOURCE_TYPE::BUFFER)
        {
            auto internal_state_src = to_internal((const GraphicsBuffer*) pSrc);
            auto internal_state_dst = to_internal((const GraphicsBuffer*) pDst);

            const GPUBufferDesc& src_desc = ((const GraphicsBuffer*) pSrc)->GetDesc();
            const GPUBufferDesc& dst_desc = ((const GraphicsBuffer*) pDst)->GetDesc();

            VkBufferCopy copy = {};
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size = (VkDeviceSize) std::min(src_desc.ByteWidth, dst_desc.ByteWidth);

            vkCmdCopyBuffer(GetDirectCommandList(),
                            internal_state_src->resource,
                            internal_state_dst->resource,
                            1, &copy);
        }
    }

    GPUAllocation Vulkan_CommandList::AllocateGPU(const uint32_t size)
    {
        ALIMER_ASSERT_MSG(size > 0, "Allocation size must be greater than zero");

        GPUAllocation result{};
        if (size == 0)
        {
            return result;
        }

        ResourceFrameAllocator& allocator = resourceBuffer[frameIndex];
        VkDeviceSize minUniformBufferOffsetAlignment = device->device_properties.properties.limits.minUniformBufferOffsetAlignment;
        uint8_t* dest = allocator.Allocate(size, minUniformBufferOffsetAlignment);
        assert(dest != nullptr);

        result.buffer = allocator.buffer;
        result.offset = (uint32_t) allocator.calculateOffset(dest);
        result.data = (void*) dest;
        return result;
    }

    void Vulkan_CommandList::UpdateBuffer(GraphicsBuffer* buffer, const void* data, uint64_t size)
    {
        const GPUBufferDesc& bufferDesc = buffer->GetDesc();

        assert(bufferDesc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
        assert(bufferDesc.ByteWidth >= size && "Data size is too big!");

        auto internal_state = to_internal(buffer);

        if (size == 0)
        {
            size = bufferDesc.ByteWidth;
        }
        else
        {
            size = alimer::Min<uint64_t>(bufferDesc.ByteWidth, size);
        }

        if (bufferDesc.Usage == USAGE_DYNAMIC && bufferDesc.BindFlags & BIND_CONSTANT_BUFFER)
        {
            // Dynamic buffer will be used from host memory directly:
            GPUAllocation allocation = AllocateGPU(size);
            memcpy(allocation.data, data, size);
            internal_state->dynamic[frameIndex] = allocation;
            descriptors[frameIndex].dirty = true;
        }
        else
        {
            // Contents will be transferred to device memory:

            // barrier to transfer:

            assert(active_renderpass == nullptr); // must not be inside render pass

            VkPipelineStageFlags stages = 0;

            VkBufferMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.buffer = internal_state->resource;
            barrier.srcAccessMask = 0;
            if (bufferDesc.BindFlags & BIND_CONSTANT_BUFFER)
            {
                barrier.srcAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
                stages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (bufferDesc.BindFlags & BIND_VERTEX_BUFFER)
            {
                barrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
                stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            }
            if (bufferDesc.BindFlags & BIND_INDEX_BUFFER)
            {
                barrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
                stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            }
            if (bufferDesc.BindFlags & BIND_SHADER_RESOURCE)
            {
                barrier.srcAccessMask |= VK_ACCESS_SHADER_READ_BIT;
                stages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (bufferDesc.BindFlags & BIND_UNORDERED_ACCESS)
            {
                barrier.srcAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
                stages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (bufferDesc.MiscFlags & RESOURCE_MISC_RAY_TRACING)
            {
                barrier.srcAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                stages = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
            }
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.size = VK_WHOLE_SIZE;

            vkCmdPipelineBarrier(
                GetDirectCommandList(),
                stages,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr);

            // issue data copy:
            uint8_t* dest = resourceBuffer[frameIndex].Allocate(size, 1);
            memcpy(dest, data, size);

            VkBufferCopy copyRegion = {};
            copyRegion.size = size;
            copyRegion.srcOffset = resourceBuffer[frameIndex].calculateOffset(dest);
            copyRegion.dstOffset = 0;

            vkCmdCopyBuffer(GetDirectCommandList(),
                            to_internal(resourceBuffer[frameIndex].buffer.Get())->resource,
                            internal_state->resource, 1, &copyRegion);

            // reverse barrier:
            std::swap(barrier.srcAccessMask, barrier.dstAccessMask);

            vkCmdPipelineBarrier(
                GetDirectCommandList(),
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                stages,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr);
        }
    }

    void Vulkan_CommandList::QueryBegin(const GPUQuery* query)
    {
        auto internal_state = to_internal(query);

        switch (query->desc.Type)
        {
            case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
                vkCmdBeginQuery(GetDirectCommandList(), device->querypool_occlusion, (uint32_t) internal_state->query_index, 0);
                break;
            case GPU_QUERY_TYPE_OCCLUSION:
                vkCmdBeginQuery(GetDirectCommandList(), device->querypool_occlusion, (uint32_t) internal_state->query_index, VK_QUERY_CONTROL_PRECISE_BIT);
                break;
        }
    }

    void Vulkan_CommandList::QueryEnd(const GPUQuery* query)
    {
        auto internal_state = to_internal(query);

        switch (query->desc.Type)
        {
            case GPU_QUERY_TYPE_TIMESTAMP:
                vkCmdWriteTimestamp(GetDirectCommandList(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, device->querypool_timestamp, internal_state->query_index);
                break;
            case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
                vkCmdEndQuery(GetDirectCommandList(), device->querypool_occlusion, internal_state->query_index);
                break;
            case GPU_QUERY_TYPE_OCCLUSION:
                vkCmdEndQuery(GetDirectCommandList(), device->querypool_occlusion, internal_state->query_index);
                break;
        }
    }

    void Vulkan_CommandList::Barrier(const GPUBarrier* barriers, uint32_t numBarriers)
    {
        VkMemoryBarrier memorybarriers[8];
        VkImageMemoryBarrier imagebarriers[8];
        VkBufferMemoryBarrier bufferbarriers[8];
        uint32_t memorybarrier_count = 0;
        uint32_t imagebarrier_count = 0;
        uint32_t bufferbarrier_count = 0;

        for (uint32_t i = 0; i < numBarriers; ++i)
        {
            const GPUBarrier& barrier = barriers[i];

            switch (barrier.type)
            {
                default:
                case GPUBarrier::MEMORY_BARRIER:
                {
                    VkMemoryBarrier& barrierdesc = memorybarriers[memorybarrier_count++];
                    barrierdesc.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                    barrierdesc.pNext = nullptr;
                    barrierdesc.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                    barrierdesc.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
                    if (device->RAYTRACING)
                    {
                        barrierdesc.srcAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                        barrierdesc.dstAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                    }
                }
                break;
                case GPUBarrier::IMAGE_BARRIER:
                {
                    const TextureDesc& desc = barrier.image.texture->GetDesc();
                    auto internal_state = to_internal(barrier.image.texture);

                    VkImageMemoryBarrier& barrierdesc = imagebarriers[imagebarrier_count++];
                    barrierdesc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrierdesc.pNext = nullptr;
                    barrierdesc.image = internal_state->resource;
                    barrierdesc.oldLayout = _ConvertImageLayout(barrier.image.layout_before);
                    barrierdesc.newLayout = _ConvertImageLayout(barrier.image.layout_after);
                    barrierdesc.srcAccessMask = _ParseImageLayout(barrier.image.layout_before);
                    barrierdesc.dstAccessMask = _ParseImageLayout(barrier.image.layout_after);
                    if (desc.BindFlags & BIND_DEPTH_STENCIL)
                    {
                        barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        if (device->IsFormatStencilSupport(desc.format))
                        {
                            barrierdesc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                        }
                    }
                    else
                    {
                        barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    }
                    barrierdesc.subresourceRange.baseArrayLayer = 0;
                    barrierdesc.subresourceRange.layerCount = desc.ArraySize;
                    barrierdesc.subresourceRange.baseMipLevel = 0;
                    barrierdesc.subresourceRange.levelCount = desc.MipLevels;
                    barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                }
                break;
                case GPUBarrier::BUFFER_BARRIER:
                {
                    auto internal_state = to_internal(barrier.buffer.buffer);

                    VkBufferMemoryBarrier& barrierdesc = bufferbarriers[bufferbarrier_count++];
                    barrierdesc.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    barrierdesc.pNext = nullptr;
                    barrierdesc.buffer = internal_state->resource;
                    barrierdesc.size = barrier.buffer.buffer->GetDesc().ByteWidth;
                    barrierdesc.offset = 0;
                    barrierdesc.srcAccessMask = _ParseBufferState(barrier.buffer.state_before);
                    barrierdesc.dstAccessMask = _ParseBufferState(barrier.buffer.state_after);
                    barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                }
                break;
            }
        }

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        if (device->RAYTRACING)
        {
            srcStage |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
            dstStage |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        }

        vkCmdPipelineBarrier(GetDirectCommandList(),
                             srcStage,
                             dstStage,
                             0,
                             memorybarrier_count, memorybarriers,
                             bufferbarrier_count, bufferbarriers,
                             imagebarrier_count, imagebarriers);
    }

    bool IsVulkanBackendAvailable()
    {
        return GraphicsDevice_Vulkan::IsAvailable();
    }

    RefPtr<Graphics> CreateVulkanGraphics(WindowHandle window, const GraphicsSettings& settings)
    {
        return RefPtr<Graphics>(new GraphicsDevice_Vulkan(window, settings));
    }
}
