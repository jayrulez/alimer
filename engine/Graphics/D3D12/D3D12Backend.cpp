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

#include "D3D12Backend.h"

namespace alimer
{
    D3D12PlatformFunctions::D3D12PlatformFunctions()
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        dxgiLib = LoadLibraryA("dxgi.dll");
        dxgiGetDebugInterface1 = (PFN_DXGI_GET_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");
        createDxgiFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");

        d3d12Lib = LoadLibraryA("d3d12.dll");
        d3d12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12Lib, "D3D12CreateDevice");
        d3d12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12Lib, "D3D12GetDebugInterface");
#else
        dxgiGetDebugInterface1 = DXGIGetDebugInterface1;
        createDxgiFactory2 = CreateDXGIFactory2;
        d3d12CreateDevice = D3D12CreateDevice;
        d3d12GetDebugInterface = D3D12GetDebugInterface;
#endif
    }

    D3D12PlatformFunctions::~D3D12PlatformFunctions()
    {

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        FreeLibrary(dxgiLib);
        FreeLibrary(d3d12Lib);
#endif
    }
}
