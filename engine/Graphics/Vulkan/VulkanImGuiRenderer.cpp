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

#include "AlimerConfig.h"
#if defined(ALIMER_IMGUI)
#    include "Core/Log.h"
#    include "UI/backends/imgui_impl_vulkan.h"
#    include "VulkanBackend.h"
#    include "VulkanImGuiRenderer.h"

namespace alimer
{
    VulkanImGuiRenderer::VulkanImGuiRenderer(VulkanGraphics& graphics, uint32_t imageCount)
        : graphics{graphics}
    {
        ImGuiIO& io = ImGui::GetIO();
        (void) io;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
        }

        ImGui_ImplVulkan_InitInfo imguiInitInfo = {};
        imguiInitInfo.Instance = graphics.GetVkInstance();
        imguiInitInfo.PhysicalDevice = graphics.GetVkPhysicalDevice();
        imguiInitInfo.Device = graphics.GetVkDevice();
        //imguiInitInfo.QueueFamily = queueFamilies.graphicsQueueFamilyIndex;
        //imguiInitInfo.Queue = graphicsQueue;

        imguiInitInfo.MinImageCount = imageCount;
        imguiInitInfo.ImageCount = imageCount;

        //imguiInitInfo.DescriptorPool = m_guiDescriptorPool;
        imguiInitInfo.PipelineCache = VK_NULL_HANDLE;
        imguiInitInfo.Allocator = nullptr;

        imguiInitInfo.CheckVkResultFn = [](VkResult result) {
            if (result != VK_SUCCESS)
            {
                LOGE("DearImGui vulkan error!");
            }
        };

        //VkRenderPass renderPass = VK_NULL_HANDLE;
        //ImGui_ImplVulkan_Init(&imguiInitInfo, renderPass);
        //ImGui_ImplVulkan_SetMinImageCount(imageCount);
    }

    VulkanImGuiRenderer::~VulkanImGuiRenderer()
    {
    }

    void VulkanImGuiRenderer::Shutdown()
    {
        ImGui_ImplVulkan_Shutdown();
    }
}

#endif /* defined(ALIMER_IMGUI) */