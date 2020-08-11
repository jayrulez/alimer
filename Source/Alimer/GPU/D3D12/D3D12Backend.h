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

#include "Core/Assert.h"
#include "Core/Log.h"
#include "GPU/Types.h"

// Use the C++ standard templated min/max
#define NOMINMAX

#if !ALIMER_PLATFORM_UWP && !ALIMER_PLATFORM_XBOXONE
// DirectX apps don't need GDI
#   define NODRAWTEXT
#   define NOGDI
#   define NOBITMAP

// Include <mcx.h> if you need this
#   define NOMCX

// Include <winsvc.h> if you need this
#   define NOSERVICE

// WinHelp is deprecated
#   define NOHELP

#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif

#include <wrl/client.h>
#include <wrl/event.h>

#include <d3d12.h>

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#ifdef _DEBUG
#   include <dxgidebug.h>

// Declare Guids to avoid linking with "dxguid.lib"
static constexpr GUID D3D12_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8 } };
static constexpr GUID D3D12_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a } };
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, _COM_Outptr_ void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    extern PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    extern PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
#endif

    // Type alias for Win32 ComPtr template
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }
}

#if TODO


    class D3D12GraphicsDevice;

    struct D3D12MapResult
    {
        void* CPUAddress = nullptr;
        uint64_t GPUAddress = 0;
        uint64_t ResourceOffset = 0;
        ID3D12Resource* Resource = nullptr;
    };

    class D3D12GpuResource
    {
    public:
        D3D12GpuResource() noexcept
            : resource(nullptr)
            , state(D3D12_RESOURCE_STATE_COMMON)
            , transitioningState((D3D12_RESOURCE_STATES)-1)
            , gpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {
        }

        D3D12GpuResource(ID3D12Resource* resource_, D3D12_RESOURCE_STATES currentState)
            : resource(resource_)
            , state(currentState)
            , transitioningState((D3D12_RESOURCE_STATES)-1)
            , gpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {
        }

        virtual void Destroy()
        {
            gpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        }

        D3D12_RESOURCE_STATES GetState() const { return state; }
        void SetState(D3D12_RESOURCE_STATES newState) { state = newState; }

        D3D12_RESOURCE_STATES GetTransitioningState() const { return transitioningState; }
        void SetTransitioningState(D3D12_RESOURCE_STATES newState) { transitioningState = newState; }

        //ID3D12Resource* operator->() { return resource.Get(); }
        //const ID3D12Resource* operator->() const { return resource.Get(); }

        //ID3D12Resource* GetResource() { return resource.Get(); }
        //const ID3D12Resource* GetResource() const { return resource.Get(); }


        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return gpuVirtualAddress; }

    protected:
        ID3D12Resource* resource = nullptr;
        D3D12_RESOURCE_STATES state;
        D3D12_RESOURCE_STATES transitioningState;
        D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
    };

    struct DescriptorHeap
    {
        ID3D12DescriptorHeap* handle;
        D3D12_CPU_DESCRIPTOR_HANDLE CpuStart;
        D3D12_GPU_DESCRIPTOR_HANDLE GpuStart;
        uint32_t Size;
        uint32_t Capacity;
        uint32_t DescriptorSize;
    };

    DescriptorHeap D3D12CreateDescriptorHeap(ID3D12Device* device, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
}

#endif // TODO
