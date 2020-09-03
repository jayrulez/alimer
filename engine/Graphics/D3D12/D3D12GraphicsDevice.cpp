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

#include "D3D12GraphicsDevice.h"
//#include "D3D12Texture.h"
#include "D3D12Buffer.h"
//#include "D3D12CommandContext.h"

#if defined(_DEBUG)
#   define ENABLE_GPU_VALIDATION 0
#else
#   define ENABLE_GPU_VALIDATION 0
#endif

namespace Alimer
{
    bool D3D12GraphicsDevice::IsAvailable()
    {
        static bool available = false;
        static bool available_initialized = false;

        if (available_initialized) {
            return available;
        }

        available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        static HMODULE dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!dxgiDLL) {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiDLL, "CreateDXGIFactory2");
        if (!CreateDXGIFactory2)
        {
            return false;
        }
        DXGIGetDebugInterface1 = (PFN_DXGI_GET_DEBUG_INTERFACE1)GetProcAddress(dxgiDLL, "DXGIGetDebugInterface1");

        static HMODULE d3d12DLL = LoadLibraryA("d3d12.dll");
        if (!d3d12DLL) {
            return false;
        }

        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12DLL, "D3D12GetDebugInterface");
        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12DLL, "D3D12CreateDevice");
        if (!D3D12CreateDevice) {
            return false;
        }
#endif

        available = true;
        return true;
    }

    D3D12GraphicsDevice::D3D12GraphicsDevice(const GraphicsDeviceDescription& desc)
        : dxgiFactoryCaps(DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR)
    {
        ALIMER_VERIFY(IsAvailable());
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        Shutdown();
    }

    void D3D12GraphicsDevice::Shutdown()
    {

    }

    void D3D12GraphicsDevice::HandleDeviceLost()
    {

    }


    void D3D12GraphicsDevice::InitCapabilities(IDXGIAdapter1* adapter)
    {

    }

    void D3D12GraphicsDevice::WaitForGPU()
    {

    }

    uint64 D3D12GraphicsDevice::Frame()
    {
        return frameCount;
    }
}
