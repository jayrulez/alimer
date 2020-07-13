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

#include "Graphics/Gui.h"
#include "Core/Window.h"
#include "imgui_impl_glfw.h"

namespace alimer
{
    namespace
    {
        struct VERTEX_CONSTANT_BUFFER
        {
            float   mvp[4][4];
        };
    }

    Gui::Gui(Window* window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.UserData = this;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 18.0f);

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        // Color scheme
        /*style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
        style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);*/

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        io.BackendRendererName = "vgpu";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        //io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

        // Init imgui stuff
        ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window->GetWindow(), true);

        /*ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->RendererUserData = IM_NEW(ImGuiViewportDataD3D12)(maxInflightFrames);

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            platform_io.Renderer_CreateWindow = ImGui_D3D12_CreateWindow;
            platform_io.Renderer_DestroyWindow = ImGui_D3D12_DestroyWindow;
            platform_io.Renderer_SetWindowSize = ImGui_D3D12_SetWindowSize;
            platform_io.Renderer_RenderWindow = ImGui_D3D12_RenderWindow;
            platform_io.Renderer_SwapBuffers = ImGui_D3D12_SwapBuffers;
        }*/
    }

    Gui::~Gui()
    {
        //vgpu::destroyTexture(_fontTexture);
        //vgpu::DestroyShader(_shader);
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void Gui::BeginFrame()
    {
        // Start the Dear ImGui frame
        //if (!_fontTexture.isValid())
        {
            ImGuiIO& io = ImGui::GetIO();
            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            // Upload texture to graphics system
            /*vgpu::TextureDescription textureDesc = {};
            textureDesc.label = "ImGui FontTexture";
            textureDesc.type = vgpu::TextureType::Type2D;
            textureDesc.format = vgpu::PixelFormat::RGBA8Unorm;
            textureDesc.width = width;
            textureDesc.height = height;
            _fontTexture = vgpu::createTexture(textureDesc, pixels);

            // Store our identifier
            static_assert(sizeof(ImTextureID) >= sizeof(_fontTexture), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
            io.Fonts->TexID = (ImTextureID)(intptr_t)_fontTexture.id;

            static const char* shaderSource =
                "cbuffer vertexBuffer : register(b0) \
            {\
              float4x4 ProjectionMatrix; \
            };\
            \
            sampler sampler0;\
            Texture2D texture0;\
            \
            struct VS_INPUT\
            {\
              float2 pos : POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT VSMain(VS_INPUT input)\
            {\
              PS_INPUT output;\
              output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
              output.col = input.col;\
              output.uv  = input.uv;\
              return output;\
            }\
            float4 PSMain(PS_INPUT input) : SV_Target \
            {\
              float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
              return out_col; \
            }";

            vgpu::ShaderDescription shaderDesc = {};
            shaderDesc.stages[(uint32_t)vgpu::ShaderStage::Vertex] = vgpu::CompileShader(shaderSource, "VSMain", vgpu::ShaderStage::Vertex);
            shaderDesc.stages[(uint32_t)vgpu::ShaderStage::Fragment] = vgpu::CompileShader(shaderSource, "PSMain", vgpu::ShaderStage::Fragment);

            _shader = vgpu::CreateShader(shaderDesc);

            _uniformBuffer = vgpu::CreateBuffer(sizeof(VERTEX_CONSTANT_BUFFER), vgpu::BufferUsage::Uniform);*/
        }

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Gui::Render()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Render();
        //RenderDrawData(ImGui::GetDrawData(), 0);

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

#if TODO
    void Gui::SetupRenderState(ImDrawData* drawData, vgpu::CommandList commandList)
    {
        // Setup viewport
        vgpu::cmdSetViewport(commandList, 0.0f, 0.0f, drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, 1.0f);
        vgpu::SetVertexBuffer(commandList, _vertexBuffer);
        vgpu::SetIndexBuffer(commandList, _indexBuffer);
        vgpu::SetShader(commandList, _shader);
        vgpu::BindUniformBuffer(commandList, 0, _uniformBuffer);
    }

    void Gui::RenderDrawData(ImDrawData* drawData, vgpu::CommandList commandList)
    {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        if (drawData->CmdListsCount == 0)
            return;

        // Create and grow vertex/index buffers if needed
        if (!_vertexBuffer.isValid() || _vertexBufferSize < drawData->TotalVtxCount)
        {
            vgpu::DestroyBuffer(_vertexBuffer);
            _vertexBufferSize = drawData->TotalVtxCount + 5000;

            _vertexBuffer = vgpu::CreateBuffer(_vertexBufferSize * sizeof(ImDrawVert), vgpu::BufferUsage::Vertex, sizeof(ImDrawVert));
        }

        if (!_indexBuffer.isValid() || _indexBufferSize < drawData->TotalIdxCount)
        {
            vgpu::DestroyBuffer(_indexBuffer);

            _indexBufferSize = drawData->TotalIdxCount + 10000;
            _indexBuffer = vgpu::CreateBuffer(_indexBufferSize * sizeof(ImDrawIdx), vgpu::BufferUsage::Index, sizeof(ImDrawIdx));
        }

        // Upload vertex/index data into a single contiguous GPU buffer
        ImDrawVert* vtx_dst = (ImDrawVert*)vgpu::MapBuffer(_vertexBuffer);
        ImDrawIdx* idx_dst = (ImDrawIdx*)vgpu::MapBuffer(_indexBuffer);
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = drawData->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        vgpu::UnmapBuffer(_vertexBuffer);
        vgpu::UnmapBuffer(_indexBuffer);

        {
            VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)vgpu::MapBuffer(_uniformBuffer);
            float L = drawData->DisplayPos.x;
            float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
            float T = drawData->DisplayPos.y;
            float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
            float mvp[4][4] =
            {
                { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
                { 0.0f,         0.0f,           0.5f,       0.0f },
                { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
            };
            memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
            vgpu::UnmapBuffer(_uniformBuffer);
        }

        SetupRenderState(drawData, commandList);

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_idx_offset = 0;
        int global_vtx_offset = 0;
        ImVec2 clip_off = drawData->DisplayPos;
        ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = drawData->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != NULL)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                        SetupRenderState(drawData, commandList);
                    else
                        pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    // Apply scissor/clipping rectangle
                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clip_rect;
                    clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                    clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                    clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                    clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                    if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                    {
                        // Negative offsets are illegal for vkCmdSetScissor
                        if (clip_rect.x < 0.0f)
                            clip_rect.x = 0.0f;
                        if (clip_rect.y < 0.0f)
                            clip_rect.y = 0.0f;

                        vgpu::SetScissorRect(commandList,
                            (uint32_t)clip_rect.x,
                            (uint32_t)clip_rect.y,
                            uint32_t(clip_rect.z - clip_rect.x),
                            uint32_t(clip_rect.w - clip_rect.y)
                        );

                        // Bind texture, Draw
                        vgpu::TextureHandle textureId = { (uint32_t)(intptr_t)pcmd->TextureId };
                        vgpu::BindTexture(commandList, 0, textureId);
                        vgpu::DrawIndexed(commandList, pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
                    }
                }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
        }
    }
#endif // TODO

}
