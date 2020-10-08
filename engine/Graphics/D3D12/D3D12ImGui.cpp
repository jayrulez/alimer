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

#include "AlimerConfig.h"

#if defined(ALIMER_IMGUI)
#include "D3D12ImGui.h"
#include "Graphics/SwapChain.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    namespace
    {
        struct ImGuiViewportDataD3D12
        {
            SwapChain* swapChain;

            ImGuiViewportDataD3D12()
            {
                swapChain = nullptr;
            }

            ~ImGuiViewportDataD3D12()
            {
                IM_ASSERT(swapChain == nullptr);
            };
        };

        static void ImGui_D3D12_CreateWindow(ImGuiViewport* viewport)
        {
            ImGuiIO& io = ImGui::GetIO();
            GraphicsDevice* device = static_cast<GraphicsDevice*>(io.UserData);

            ImGuiViewportDataD3D12* data = IM_NEW(ImGuiViewportDataD3D12)();
            viewport->RendererUserData = data;

            // PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
            // Some back-ends will leave PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the HWND.
            HWND hwnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;
            IM_ASSERT(hwnd != nullptr);

            /*PresentationParameters presentationParameters = {};
            presentationParameters.handle = hwnd;
            presentationParameters.backBufferWidth = (uint32)viewport->Size.x;
            presentationParameters.backBufferHeight = (uint32)viewport->Size.y;
            presentationParameters.verticalSync = false;
            data->swapChain = new SwapChain(device, presentationParameters);*/
        }
    }

    D3D12ImGui::D3D12ImGui(D3D12GraphicsDevice* device)
        : device{ device }
    {
        ImGuiIO& io = ImGui::GetIO();
        io.UserData = device;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            platform_io.Renderer_CreateWindow = ImGui_D3D12_CreateWindow;
            //platform_io.Renderer_DestroyWindow = ImGui_ImplDX12_DestroyWindow;
            //platform_io.Renderer_SetWindowSize = ImGui_ImplDX12_SetWindowSize;
            //platform_io.Renderer_RenderWindow = ImGui_ImplDX12_RenderWindow;
            //platform_io.Renderer_SwapBuffers = ImGui_ImplDX12_SwapBuffers;
        }
    }

    D3D12ImGui::~D3D12ImGui()
    {
        ImGui::DestroyPlatformWindows();
    }
}
#endif /* defined(ALIMER_IMGUI) */
