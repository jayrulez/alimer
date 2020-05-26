#if defined(GPU_DRIVER_D3D11)
#include "gpu_backend.h"

#define D3D11_NO_HELPERS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif


#endif /* defined(GPU_DRIVER_D3D11) */
