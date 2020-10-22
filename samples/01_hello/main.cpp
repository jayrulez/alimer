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

#include "Platform/Application.h"
#include "IO/FileSystem.h"
#include "RHI/RHI.h"
#include "Core/Math.h"
#include "Math/Color.h"
#include "Math/Matrix4x4.h"

namespace Alimer
{
    class HelloWorldApp final : public Application
    {
    public:
        HelloWorldApp(const Config& config)
            : Application(config)
        {

        }
        ~HelloWorldApp() override;

        void Initialize() override;
        void OnDraw(CommandList commandList) override;

    private:
        RefPtr<GraphicsBuffer> vertexBuffer;
        RefPtr<GraphicsBuffer> indexBuffer;
        RefPtr<GraphicsBuffer> constantBuffer;
        PipelineState pipeline;
    };

    HelloWorldApp::~HelloWorldApp()
    {
    }

    struct Vertex
    {
        Float3 position;
        Color color;
        Float2 uv;
    };

    /* TODO: Until we fix resource creation */
    Shader vertexShader;
    Shader pixelShader;
    Texture texture;
    RefPtr<Sampler> sampler;

    void HelloWorldApp::Initialize()
    {
        auto shaderSource = File::ReadAllText("assets/Shaders/triangle.hlsl");
        graphicsDevice->CreateShader(ShaderStage::Vertex, shaderSource.c_str(), "VSMain", &vertexShader);
        graphicsDevice->CreateShader(ShaderStage::Fragment, shaderSource.c_str(), "PSMain", &pixelShader);

        RenderPipelineDescriptor renderPipelineDesc = {};
        renderPipelineDesc.vs = &vertexShader;
        renderPipelineDesc.ps = &pixelShader;
        renderPipelineDesc.vertexDescriptor.attributes[0].format = VertexFormat::Float3;
        renderPipelineDesc.vertexDescriptor.attributes[1].format = VertexFormat::Float4;
        renderPipelineDesc.vertexDescriptor.attributes[2].format = VertexFormat::Float2;
        /*renderPipelineDesc.colorAttachments[0].blendEnable = true;
        renderPipelineDesc.colorAttachments[0].srcColorBlendFactor = BlendFactor::One;
        renderPipelineDesc.colorAttachments[0].dstColorBlendFactor = BlendFactor::SourceAlpha;
        renderPipelineDesc.colorAttachments[0].srcAlphaBlendFactor = BlendFactor::One;
        renderPipelineDesc.colorAttachments[0].dstAlphaBlendFactor = BlendFactor::OneMinusSourceAlpha;*/
        graphicsDevice->CreateRenderPipeline(&renderPipelineDesc, &pipeline);

        uint32_t pixels[4 * 4] = {
            0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
            0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
            0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
            0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
        };
        TextureDesc textureDesc = {};
        textureDesc.Width = 4;
        textureDesc.Height = 4;
        textureDesc.Format = FORMAT_R8G8B8A8_UNORM;
        textureDesc.BindFlags = BIND_SHADER_RESOURCE;
        SubresourceData textureData = {};
        textureData.pSysMem = pixels;
        textureData.SysMemPitch = 4u * graphicsDevice->GetFormatStride(textureDesc.Format);
        graphicsDevice->CreateTexture(&textureDesc, &textureData, &texture);

        SamplerDescriptor samplerDesc = {};
        samplerDesc.minFilter = FilterMode::Nearest;
        samplerDesc.magFilter = FilterMode::Nearest;
        samplerDesc.mipFilter = FilterMode::Nearest;
        sampler = graphicsDevice->CreateSampler(&samplerDesc);

        /*Vertex quadVertices[] =
        {
            { { -0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { 0.5f, -0.5, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
            { { -0.5f, -0.5, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        };*/

        float cubeData[] =
        {
            /* pos                  color                       uvs */
            -1.0f, -1.0f, -1.0f,    1.0f, 0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
             1.0f, -1.0f, -1.0f,    1.0f, 0.0f, 0.0f, 1.0f,     1.0f, 0.0f,
             1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.0f, 1.0f,     1.0f, 1.0f,
            -1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.0f, 1.0f,     0.0f, 1.0f,

            -1.0f, -1.0f,  1.0f,    0.0f, 1.0f, 0.0f, 1.0f,     0.0f, 0.0f,
             1.0f, -1.0f,  1.0f,    0.0f, 1.0f, 0.0f, 1.0f,     1.0f, 0.0f,
             1.0f,  1.0f,  1.0f,    0.0f, 1.0f, 0.0f, 1.0f,     1.0f, 1.0f,
            -1.0f,  1.0f,  1.0f,    0.0f, 1.0f, 0.0f, 1.0f,     0.0f, 1.0f,

            -1.0f, -1.0f, -1.0f,    0.0f, 0.0f, 1.0f, 1.0f,     0.0f, 0.0f,
            -1.0f,  1.0f, -1.0f,    0.0f, 0.0f, 1.0f, 1.0f,     1.0f, 0.0f,
            -1.0f,  1.0f,  1.0f,    0.0f, 0.0f, 1.0f, 1.0f,     1.0f, 1.0f,
            -1.0f, -1.0f,  1.0f,    0.0f, 0.0f, 1.0f, 1.0f,     0.0f, 1.0f,

             1.0f, -1.0f, -1.0f,    1.0f, 0.5f, 0.0f, 1.0f,     0.0f, 0.0f,
             1.0f,  1.0f, -1.0f,    1.0f, 0.5f, 0.0f, 1.0f,     1.0f, 0.0f,
             1.0f,  1.0f,  1.0f,    1.0f, 0.5f, 0.0f, 1.0f,     1.0f, 1.0f,
             1.0f, -1.0f,  1.0f,    1.0f, 0.5f, 0.0f, 1.0f,     0.0f, 1.0f,

            -1.0f, -1.0f, -1.0f,    0.0f, 0.5f, 1.0f, 1.0f,     0.0f, 0.0f,
            -1.0f, -1.0f,  1.0f,    0.0f, 0.5f, 1.0f, 1.0f,     1.0f, 0.0f,
             1.0f, -1.0f,  1.0f,    0.0f, 0.5f, 1.0f, 1.0f,     1.0f, 1.0f,
             1.0f, -1.0f, -1.0f,    0.0f, 0.5f, 1.0f, 1.0f,     0.0f, 1.0f,

            -1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.5f, 1.0f,     0.0f, 0.0f,
            -1.0f,  1.0f,  1.0f,    1.0f, 0.0f, 0.5f, 1.0f,     1.0f, 0.0f,
             1.0f,  1.0f,  1.0f,    1.0f, 0.0f, 0.5f, 1.0f,     1.0f, 1.0f,
             1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.5f, 1.0f,     0.0f, 1.0f
        };

        GPUBufferDesc bufferDesc{};
        bufferDesc.Usage = USAGE_IMMUTABLE;
        bufferDesc.BindFlags = BIND_VERTEX_BUFFER;
        bufferDesc.ByteWidth = sizeof(cubeData);
        //bufferDesc.StructureByteStride = sizeof(Vertex);
        vertexBuffer = graphicsDevice->CreateBuffer(bufferDesc, cubeData);

        // Index buffer
        const uint16_t indices[] = {
            0, 1, 2,  0, 2, 3,
            6, 5, 4,  7, 6, 4,
            8, 9, 10,  8, 10, 11,
            14, 13, 12,  15, 14, 12,
            16, 17, 18,  16, 18, 19,
            22, 21, 20, 23, 22, 20
        };

        bufferDesc.Usage = USAGE_IMMUTABLE;
        bufferDesc.BindFlags = BIND_INDEX_BUFFER;
        bufferDesc.ByteWidth = sizeof(indices);
        indexBuffer = graphicsDevice->CreateBuffer(bufferDesc, indices);

        GPUBufferDesc bd;
        bd.Usage = USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(Matrix4x4);
        bd.BindFlags = BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = CPU_ACCESS_WRITE;

        constantBuffer = graphicsDevice->CreateBuffer(bd, nullptr);
    }

    void HelloWorldApp::OnDraw(CommandList commandList)
    {
        static float time = 0.0f;
        XMMATRIX world = XMMatrixRotationX(time) * XMMatrixRotationY(time * 2) * XMMatrixRotationZ(time * .7f);

        float aspect = (float)(GetMainWindow()->GetSize().width / GetMainWindow()->GetSize().height);
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, 5, 1), XMVectorZero(), XMVectorSet(0, 1, 0, 1));
        XMMATRIX proj = XMMatrixPerspectiveFovLH(Pi / 4.0f, aspect, 0.1f, 100);
        XMMATRIX viewProj = XMMatrixMultiply(world, XMMatrixMultiply(view, proj));

        //Matrix4x4 projectionMatrix;
        //Matrix4x4::CreatePerspectiveFieldOfView(Pi / 4.0f, aspect, 0.1f, 100, &projectionMatrix);

        XMFLOAT4X4 worldViewProjection;
        XMStoreFloat4x4(&worldViewProjection, viewProj);
        graphicsDevice->UpdateBuffer(commandList, constantBuffer, &worldViewProjection);

        const GraphicsBuffer* vbs[] = {
            vertexBuffer,
        };

        uint32_t stride = sizeof(Vertex);
        graphicsDevice->BindVertexBuffers(vbs, 0, 1, &stride, nullptr, commandList);
        graphicsDevice->BindIndexBuffer(indexBuffer, IndexFormat::UInt16, 0, commandList);
        graphicsDevice->BindPipelineState(&pipeline, commandList);
        graphicsDevice->BindConstantBuffer(ShaderStage::Vertex, constantBuffer, 0, commandList);
        graphicsDevice->BindResource(ShaderStage::Fragment, &texture, 0, commandList);
        graphicsDevice->BindSampler(ShaderStage::Fragment, sampler, 0, commandList);
        graphicsDevice->DrawIndexed(36, 0, 0, commandList);
        /*context->PushDebugGroup("Frame");
        RenderPassDesc renderPass{};
        renderPass.colorAttachments[0].texture = windowSwapChain->GetCurrentTexture();
        renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;
        context->BeginRenderPass(renderPass);
        context->EndRenderPass();
        context->PopDebugGroup();*/

        time += 0.03f;
    }

    Application* CreateApplication()
    {
        Config config{};
        config.preferredBackendType = GraphicsBackendType::Direct3D11;
        //config.preferredBackendType = GraphicsBackendType::Direct3D12;
        //config.preferredBackendType = GraphicsBackendType::Vulkan;
        config.title = "Spinning Cube";
        //config.fullscreen = true;
        //config.width = 1280;
        //config.height = 720;

        return new HelloWorldApp(config);
    }
}

