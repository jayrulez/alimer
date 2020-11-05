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

#include "Core/Math.h"
#include "Graphics/CommandBuffer.h"
#include "Graphics/Graphics.h"
#include "IO/FileSystem.h"
#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Platform/Application.h"
#include <DirectXMath.h>

namespace alimer
{
    class HelloWorldApp final : public Application
    {
    public:
        HelloWorldApp(const Config& config)
            : Application(config) {}
        ~HelloWorldApp() override;

        void Initialize() override;
        void OnDraw(CommandBuffer& commandBuffer) override;

    private:
        // UniquePtr<Window> window2;
        // RefPtr<SwapChain> swapChain2;

        RefPtr<GraphicsBuffer> vertexBuffer;
        RefPtr<GraphicsBuffer> indexBuffer;
        RefPtr<GraphicsBuffer> constantBuffer;
        RefPtr<RenderPipeline> pipeline;
        RefPtr<Texture> texture;
        RefPtr<Sampler> sampler;
    };

    /* TODO: Until we fix resource creation */
    Shader* vertexShader;
    Shader* pixelShader;

    HelloWorldApp::~HelloWorldApp()
    {
        delete vertexShader;
        delete pixelShader;
    }

    struct Vertex
    {
        Float3 position;
        Color color;
        Float2 uv;
    };

    void HelloWorldApp::Initialize()
    {
        // window2 = MakeUnique<Window>("Window 2");
        // swapChain2 = graphicsDevice->CreateSwapChain(window2->GetHandle());
        // swapChain2->SetVerticalSync(false);

        auto shaderSource = File::ReadAllText("assets/Shaders/triangle.hlsl");
        vertexShader = new Shader();
        pixelShader = new Shader();
        graphics->CreateShader(ShaderStage::Vertex, shaderSource.c_str(), "VSMain", vertexShader);
        graphics->CreateShader(ShaderStage::Fragment, shaderSource.c_str(), "PSMain", pixelShader);

        RenderPipelineDescriptor renderPipelineDesc = {};
        renderPipelineDesc.vs = vertexShader;
        renderPipelineDesc.ps = pixelShader;
        renderPipelineDesc.vertexDescriptor.attributes[0].format = VertexFormat::Float3;
        renderPipelineDesc.vertexDescriptor.attributes[1].format = VertexFormat::Float4;
        renderPipelineDesc.vertexDescriptor.attributes[2].format = VertexFormat::Float2;
        renderPipelineDesc.colorAttachments[0].format = graphics->GetBackBufferFormat();
        // renderPipelineDesc.colorAttachments[0].blendEnable = true;
        // renderPipelineDesc.colorAttachments[0].srcColorBlendFactor = BlendFactor::One;
        // renderPipelineDesc.colorAttachments[0].dstColorBlendFactor = BlendFactor::SourceAlpha;
        // renderPipelineDesc.colorAttachments[0].srcAlphaBlendFactor = BlendFactor::One;
        // renderPipelineDesc.colorAttachments[0].dstAlphaBlendFactor = BlendFactor::OneMinusSourceAlpha;
        pipeline = graphics->CreateRenderPipeline(&renderPipelineDesc);

        uint32_t pixels[4 * 4] = {
            0xFFFFFFFF,
            0x00000000,
            0xFFFFFFFF,
            0x00000000,
            0x00000000,
            0xFFFFFFFF,
            0x00000000,
            0xFFFFFFFF,
            0xFFFFFFFF,
            0x00000000,
            0xFFFFFFFF,
            0x00000000,
            0x00000000,
            0xFFFFFFFF,
            0x00000000,
            0xFFFFFFFF,
        };

        TextureDescription textureDesc = TextureDescription::Texure2D(PixelFormat::RGBA8Unorm, 4, 4, 1u);

        SubresourceData textureData = {};
        textureData.pSysMem = pixels;
        textureData.SysMemPitch = 4u * GetFormatBlockSize(textureDesc.format);
        texture = graphics->CreateTexture(&textureDesc, &textureData);

        SamplerDescriptor samplerDesc = {};
        samplerDesc.minFilter = FilterMode::Nearest;
        samplerDesc.magFilter = FilterMode::Nearest;
        samplerDesc.mipmapFilter = FilterMode::Nearest;
        sampler = graphics->CreateSampler(&samplerDesc);

        /*Vertex quadVertices[] =
        {
            { { -0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { 0.5f, -0.5, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
            { { -0.5f, -0.5, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        };*/

        float cubeData[] = {
            /* pos                  color                       uvs */
            -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,

            -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,

            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,

            1.0f, -1.0f, -1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f,

            -1.0f, -1.0f, -1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f,

            -1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f, 1.0f};

        GPUBufferDesc bufferDesc{};
        bufferDesc.Usage = USAGE_IMMUTABLE;
        bufferDesc.BindFlags = BIND_VERTEX_BUFFER;
        bufferDesc.ByteWidth = sizeof(cubeData);
        // bufferDesc.StructureByteStride = sizeof(Vertex);
        vertexBuffer = graphics->CreateBuffer(bufferDesc, cubeData);

        // Index buffer
        const uint16_t indices[] = {0, 1, 2, 0, 2, 3, 6, 5, 4, 7, 6, 4, 8, 9, 10, 8, 10, 11,
                                    14, 13, 12, 15, 14, 12, 16, 17, 18, 16, 18, 19, 22, 21, 20, 23, 22, 20};

        bufferDesc.Usage = USAGE_IMMUTABLE;
        bufferDesc.BindFlags = BIND_INDEX_BUFFER;
        bufferDesc.ByteWidth = sizeof(indices);
        indexBuffer = graphics->CreateBuffer(bufferDesc, indices);

        GPUBufferDesc bd;
        bd.Usage = USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(Matrix4x4);
        bd.BindFlags = BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = CPU_ACCESS_WRITE;

        constantBuffer = graphics->CreateBuffer(bd, nullptr);
    }

    void HelloWorldApp::OnDraw(CommandBuffer& commandBuffer)
    {
        static float time = 0.0f;
        XMMATRIX world = XMMatrixRotationX(time) * XMMatrixRotationY(time * 2) * XMMatrixRotationZ(time * .7f);

        float aspect = (float) (GetWindow().GetSize().width / GetWindow().GetSize().height);
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, 5, 1), XMVectorZero(), XMVectorSet(0, 1, 0, 1));
        XMMATRIX proj = XMMatrixPerspectiveFovLH(Pi / 4.0f, aspect, 0.1f, 100);
        XMMATRIX viewProj = XMMatrixMultiply(world, XMMatrixMultiply(view, proj));

        // Matrix4x4 projectionMatrix;
        // Matrix4x4::CreatePerspectiveFieldOfView(Pi / 4.0f, aspect, 0.1f, 100, &projectionMatrix);

        XMFLOAT4X4 worldViewProjection;
        XMStoreFloat4x4(&worldViewProjection, viewProj);
        commandBuffer.UpdateBuffer(constantBuffer, &worldViewProjection);

        const GraphicsBuffer* vbs[] = {
            vertexBuffer,
        };

        uint32_t stride = sizeof(Vertex);
        commandBuffer.BindVertexBuffers(vbs, 0, 1, &stride, nullptr);
        commandBuffer.BindIndexBuffer(indexBuffer, IndexFormat::UInt16, 0);
        commandBuffer.SetRenderPipeline(pipeline);
        commandBuffer.BindConstantBuffer(ShaderStage::Vertex, constantBuffer, 0);
        commandBuffer.BindResource(ShaderStage::Fragment, texture, 0);
        commandBuffer.BindSampler(ShaderStage::Fragment, sampler, 0);
        commandBuffer.DrawIndexed(36);

        time += 0.001f;
    }

    Application* CreateApplication()
    {
        Config config{};

        // config.backendType = GraphicsBackendType::Direct3D11;
        // config.backendType = GraphicsBackendType::Direct3D12;
        config.backendType = GraphicsBackendType::Vulkan;

#ifdef _DEBUG
        // Direct3D12 has issue with debug layer
        if (config.backendType == GraphicsBackendType::Vulkan)
        {
            config.deviceFlags = GraphicsDeviceFlags::DebugRuntime;
        }
#endif

        config.title = "Spinning Cube";
        // config.fullscreen = true;
        // config.width = 1280;
        // config.height = 720;

        return new HelloWorldApp(config);
    }
}
