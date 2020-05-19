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

#include "graphics_d3d12.h"

#ifndef NOMINMAX
#   define NOMINMAX
#endif 
#if defined(_WIN32)
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3dcommon.h>
#include <dxgiformat.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <d3d12.h>
#include <wrl.h>

#if defined(_DEBUG)
#   include <dxgidebug.h>

#   if !defined(_XBOX_ONE) || !defined(_TITLE)
#   pragma comment(lib,"dxguid.lib")
#   endif
#endif

#include <vector>

#define VHR(obj) if (FAILED(hr)) { assert(false); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

namespace alimer
{
    namespace graphics
    {
        namespace d3d12
        {
            using Microsoft::WRL::ComPtr;


            struct Context
            {
                enum { MAX_COUNT = 16 };
            };

            static struct {
                bool availableInitialized = false;
                bool available = false;


#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
                HMODULE dxgiDLL = nullptr;
                HMODULE d3d12DLL = nullptr;

                PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
                PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;

                PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
                PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
                PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = nullptr;
                PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer = nullptr;
                PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature = nullptr;
                PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer = nullptr;
#endif

                ComPtr<IDXGIFactory4> factory;
                Pool<Context, Context::MAX_COUNT> contexts;
            } state;

            static bool D3D12IsSupported(void)
            {
                if (state.availableInitialized) {
                    return state.available;
                }

                state.availableInitialized = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
                state.dxgiDLL = LoadLibraryW(L"dxgi.dll");
                if (state.dxgiDLL == nullptr)
                    return false;

                state.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(state.dxgiDLL, "CreateDXGIFactory2");
                if (state.CreateDXGIFactory2 == nullptr)
                    return false;

                state.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(state.dxgiDLL, "DXGIGetDebugInterface1");

                state.d3d12DLL = LoadLibraryW(L"d3d12.dll");
                if (state.d3d12DLL == nullptr)
                    return false;

                state.D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(state.d3d12DLL, "D3D12CreateDevice");
                if (state.D3D12CreateDevice == nullptr)
                    return false;

                state.D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(state.d3d12DLL, "D3D12GetDebugInterface");
                state.D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(state.d3d12DLL, "D3D12SerializeRootSignature");
                state.D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(state.d3d12DLL, "D3D12CreateRootSignatureDeserializer");
                state.D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(state.d3d12DLL, "D3D12SerializeVersionedRootSignature");
                state.D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(state.d3d12DLL, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

                ComPtr<IDXGIFactory4> tempFactory;
                HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(tempFactory.GetAddressOf()));
                if (FAILED(hr))
                {
                    return false;
                }

                if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    return false;
                }

                state.available = true;
                return state.available;
            }

            static bool D3D12Init(const Configuration& config)
            {
                return true;
            }

            static void D3D12Shutdown()
            {
                memset(&state, 0, sizeof(state));
            }

            Renderer* CreateRenderer() {
                static Renderer renderer = { nullptr };
                renderer.IsSupported = D3D12IsSupported;
                renderer.Init = D3D12Init;
                renderer.Shutdown = D3D12Shutdown;
                return &renderer;
            }
        }
    }
}
