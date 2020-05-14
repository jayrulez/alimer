//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "TextureVK.h"
#include "GraphicsDeviceVK.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "math/math.h"

namespace alimer
{
    namespace
    {
        /* Barriers */
        static VkAccessFlags VkGetAccessMask(TextureState state, VkImageAspectFlags aspectMask)
        {
            switch (state)
            {
            case TextureState::Undefined:
            case TextureState::General:
            case TextureState::Present:
                //case VGPUTextureLayout_PreInitialized:
                return (VkAccessFlagBits)0;
            case TextureState::RenderTarget:
                if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
                {
                    return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                }

                return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            case TextureState::ShaderRead:
                return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

            case TextureState::ShaderWrite:
                return VK_ACCESS_SHADER_WRITE_BIT;

                /*case VGPUTextureLayout_ResolveDest:
                case VGPUTextureLayout_CopyDest:
                    return VK_ACCESS_TRANSFER_WRITE_BIT;
                case VGPUTextureLayout_ResolveSource:
                case VGPUTextureLayout_CopySource:
                    return VK_ACCESS_TRANSFER_READ_BIT;*/

            default:
                ALIMER_UNREACHABLE();
                return (VkAccessFlagBits)-1;
            }
        }

        VkImageLayout VkGetImageLayout(TextureState layout, VkImageAspectFlags aspectMask)
        {
            switch (layout)
            {
            case TextureState::Undefined:
                return VK_IMAGE_LAYOUT_UNDEFINED;

            case TextureState::General:
                return VK_IMAGE_LAYOUT_GENERAL;

            case TextureState::RenderTarget:
                if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }

                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                //case VGPUTextureLayout_DepthStencilReadOnly:
                //    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

            case TextureState::ShaderRead:
                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            case TextureState::ShaderWrite:
                return VK_IMAGE_LAYOUT_GENERAL;

            case TextureState::Present:
                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            default:
                ALIMER_UNREACHABLE();
                return (VkImageLayout)-1;
            }
        }

        static VkPipelineStageFlags VkGetShaderStageMask(TextureState layout, VkImageAspectFlags aspectMask, bool src)
        {
            switch (layout)
            {
            case TextureState::Undefined:
            case TextureState::General:
                assert(src);
                return src ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            case TextureState::ShaderRead:
            case TextureState::ShaderWrite:
                return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT; // #OPTME Assume the worst
            case TextureState::RenderTarget:
                if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
                {
                    return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }

                return src ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                /*case Resource::State::IndirectArg:
                    return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
                case VGPUTextureLayout_CopyDest:
                case VGPUTextureLayout_CopySource:
                case VGPUTextureLayout_ResolveDest:
                case VGPUTextureLayout_ResolveSource:
                    return VK_PIPELINE_STAGE_TRANSFER_BIT;*/
            case TextureState::Present:
                return src ? (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            default:
                ALIMER_UNREACHABLE();
                return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }
        }
    }

    TextureVK::TextureVK(GraphicsDeviceVK* device_)
        : device(device_)
        , desc()
    {

    }

    TextureVK::~TextureVK()
    {
        Destroy();
    }

    bool TextureVK::Init(const TextureDesc* pDesc, const void* initialData)
    {
        memcpy(&desc, pDesc, sizeof(desc));
        return true;
    }

    void TextureVK::InitExternal(VkImage image, const TextureDesc* pDesc)
    {
        ALIMER_ASSERT(image != VK_NULL_HANDLE);
        ALIMER_ASSERT(pDesc != nullptr);

        handle = image;
        memcpy(&desc, pDesc, sizeof(desc));
    }

    void TextureVK::Destroy()
    {

    }

    void TextureVK::Barrier(VkCommandBuffer commandBuffer, TextureState newState)
    {
        if (state == newState) {
            return;
        }

        const VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // GetVkAspectMask(texture.format);

        // Create an image barrier object
        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.pNext = nullptr;
        barrier.srcAccessMask = VkGetAccessMask(state, aspectMask);
        barrier.dstAccessMask = VkGetAccessMask(newState, aspectMask);
        barrier.oldLayout = VkGetImageLayout(state, aspectMask);
        barrier.newLayout = VkGetImageLayout(newState, aspectMask);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = handle;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkPipelineStageFlags srcStageMask = VkGetShaderStageMask(state, aspectMask, true);
        VkPipelineStageFlags dstStageMask = VkGetShaderStageMask(newState, aspectMask, false);

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStageMask,
            dstStageMask,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        state = newState;
    }

    IGraphicsDevice* TextureVK::GetDevice() const
    {
        return device;
    }
}
