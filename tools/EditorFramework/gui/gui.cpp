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

#include "config.h"
#include "gui.h"
#include "Graphics/GraphicsDevice.h"
#include "Application/Window.h"
#include "Math/Color.h"
#include "Core/Log.h"


namespace alimer
{
    static int const                    NUM_FRAMES_IN_FLIGHT = 3;

    Gui::Gui()
    {
      


        /*ID3D12Device* d3dDevice = static_cast<D3D12GraphicsDevice*>(device)->GetHandle();

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 1;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            if (d3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
                return;
        }*/

        /*ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window->GetWindow(), true);
        ImGui_ImplDX12_Init(d3dDevice, NUM_FRAMES_IN_FLIGHT,
            DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
            g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
            g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());*/
    }

    Gui::~Gui()
    {
        //ImGui_ImplDX12_Shutdown();
        //ImGui_ImplGlfw_Shutdown();
        //ImGui::DestroyContext();
    }

    void Gui::Render()
    {
        ImGuiIO& io = ImGui::GetIO();
        //auto d3dContext = static_cast<D3D12CommandContext*>(&context);
        //d3dContext->GetCommandList()->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);

        ImGui::Render();
        //ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3dContext->GetCommandList());

         // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
        }
    }
}
