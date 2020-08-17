//
// Copyright (c) 2019-2020 Amer Koleci.
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

#if defined(AGPU_DRIVER_D3D11)
#define D3D11_NO_HELPERS

#define D3D11_NO_HELPERS
#define CINTERFACE
#define COBJMACROS
#include <d3d11_1.h>
#include "agpu_driver_d3d_common.h"
//#include "stb_ds.h"

typedef struct D3D11Renderer {
    ID3D11Device1* device;
    ID3D11DeviceContext1* context;
} D3D11Renderer;

/* Device/Renderer */
static void D3D11_DestroyDevice(AGPUDevice* device)
{
    D3D11Renderer* renderer = (D3D11Renderer*)device->driverData;
    AGPU_FREE(renderer);
    AGPU_FREE(device);
}

/* Driver */
static bool D3D11_IsSupported(void)
{
    return true;
};

static AGPUDevice* D3D11_CreateDevice(void)
{
    AGPUDevice* result;
    D3D11Renderer* renderer;

    /* Allocate and zero out the renderer */
    renderer = AGPU_ALLOC(D3D11Renderer);
    memset(renderer, 0, sizeof(D3D11Renderer));

    /* Create and return the FNA3D_Device */
    result = AGPU_ALLOC(AGPUDevice);
    result->driverData = (AGPU_Renderer*)renderer;
    ASSIGN_DRIVER(D3D11);
    return result;
}

AGPU_Driver D3D11Driver = {
    AGPUBackendType_D3D11,
    D3D11_IsSupported,
    D3D11_CreateDevice
};

#endif /* defined(AGPU_DRIVER_D3D11)  */
