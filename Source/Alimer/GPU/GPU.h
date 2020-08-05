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

#pragma once

#include "Core/Ptr.h"
#include "Core/Window.h"
#include "Graphics/Types.h"
#include <EASTL/string.h>
#include <EASTL/intrusive_ptr.h>
#include <EASTL/vector.h>

namespace alimer
{
    static constexpr uint32_t kMaxCommandLists = 16u;
    using CommandList = uint16_t;

    /* Enums */
    enum class PowerPreference
    {
        Default,
        HighPerformance,
        LowPower
    };

    enum class GPUKnownVendorId
    {
        None = 0,
        AMD = 0x1002,
        Intel = 0x8086,
        Nvidia = 0x10DE,
        ARM = 0x13B5,
        ImgTec = 0x1010,
        Qualcomm = 0x5143
    };

    enum class GPUAdapterType
    {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    /* Structures */
    /// Describes GPUDevice capabilities.
    struct GPUDeviceCapabilities
    {
        RendererType backendType;
        eastl::string adapterName;
        uint32_t vendorId = 0;
        uint32_t deviceId = 0;
        GPUAdapterType adapterType = GPUAdapterType::Unknown;

        struct Features
        {
            bool independentBlend = false;
            bool computeShader = false;
            bool geometryShader = false;
            bool tessellationShader = false;
            bool logicOp = false;
            bool multiViewport = false;
            bool fullDrawIndexUint32 = false;
            bool multiDrawIndirect = false;
            bool fillModeNonSolid = false;
            bool samplerAnisotropy = false;
            bool textureCompressionETC2 = false;
            bool textureCompressionASTC_LDR = false;
            bool textureCompressionBC = false;
            /// Specifies whether cube array textures are supported.
            bool textureCubeArray = false;
            /// Specifies whether raytracing is supported.
            bool raytracing = false;
        };

        struct Limits
        {
            uint32_t maxVertexAttributes;
            uint32_t maxVertexBindings;
            uint32_t maxVertexAttributeOffset;
            uint32_t maxVertexBindingStride;
            uint32_t maxTextureDimension2D;
            uint32_t maxTextureDimension3D;
            uint32_t maxTextureDimensionCube;
            uint32_t maxTextureArrayLayers;
            uint32_t maxColorAttachments;
            uint32_t maxUniformBufferSize;
            uint32_t minUniformBufferOffsetAlignment;
            uint32_t maxStorageBufferSize;
            uint32_t minStorageBufferOffsetAlignment;
            uint32_t maxSamplerAnisotropy;
            uint32_t maxViewports;
            uint32_t maxViewportWidth;
            uint32_t maxViewportHeight;
            uint32_t maxTessellationPatchSize;
            float pointSizeRangeMin;
            float pointSizeRangeMax;
            float lineWidthRangeMin;
            float lineWidthRangeMax;
            uint32_t maxComputeSharedMemorySize;
            uint32_t maxComputeWorkGroupCountX;
            uint32_t maxComputeWorkGroupCountY;
            uint32_t maxComputeWorkGroupCountZ;
            uint32_t maxComputeWorkGroupInvocations;
            uint32_t maxComputeWorkGroupSizeX;
            uint32_t maxComputeWorkGroupSizeY;
            uint32_t maxComputeWorkGroupSizeZ;
        };

        Features features;
        Limits limits;
    };

    /* Forward declaration */
    class GPUDevice;

    /* Classes */
    class ALIMER_API GPUDevice : public RefCounted
    {
    public:
        struct Desc
        {
            RendererType preferredBackendType = RendererType::Count;
            PowerPreference powerPreference = PowerPreference::Default;
            uint32 backbufferWidth;
            uint32 backbufferHeight;
            PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
            bool enableVSync = true;
            bool isFullscreen = false;
        };

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        /// Get the device capabilities.
        ALIMER_FORCE_INLINE const GPUDeviceCapabilities& GetCaps() const { return caps; }

    protected:
        GPUDeviceCapabilities caps{};
    };

    class GPU final 
    {
    public:
        static GPUDevice* Instance;

        static void EnableBackendValidation(bool enableBackendValidation);
        static bool IsBackendValidationEnabled();
        static void EnableGPUBasedBackendValidation(bool enableGPUBasedBackendValidation);
        static bool IsGPUBasedBackendValidationEnabled();

        static GPUDevice* CreateDevice(WindowHandle windowHandle, const GPUDevice::Desc& desc);
    };
}
