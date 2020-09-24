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

#include "AlimerConfig.h"
#include "Graphics/Types.h"

#if defined(ALIMER_D3D12)
struct ID3D12Resource;
struct ID3D12Device;
struct IDXGIAdapter1;

namespace D3D12MA
{
    class Allocator;
    class Allocation;
};

#elif defined(ALIMER_VULKAN)
#   if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
typedef struct VkImage_T* VkImage;
typedef struct VkBuffer_T* VkBuffer;
#   else
typedef uint64_t VkImage;
typedef uint64_t VkBuffer;
#   endif

typedef struct VmaAllocation_T* VmaAllocation;
#endif


namespace Alimer
{
#if defined(ALIMER_D3D12)
    using GPUAllocation = D3D12MA::Allocation*;
    using TextureHandle = ID3D12Resource*;
    using BufferHandle = ID3D12Resource*;
    using PhysicalDevice = IDXGIAdapter1*;
    using DeviceHandle = ID3D12Device*;
#elif defined(ALIMER_VULKAN)
    using GPUAllocation = VmaAllocation;
    using TextureHandle = VkImage;
    using BufferHandle = VkBuffer;
    using PhysicalDevice = VkPhysicalDevice;
    using DeviceHandle = VkDevice;
#endif
}
