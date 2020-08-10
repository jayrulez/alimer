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

#include "D3D11Backend.h"

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    void D3D11SetObjectName(ID3D11DeviceChild* obj, const String& name)
    {
#ifdef _DEBUG
        if (obj != nullptr)
        {
            if (!name.empty())
            {
                obj->SetPrivateData(g_D3DDebugObjectName, static_cast<UINT>(name.length()), name.c_str());
            }
            else
                obj->SetPrivateData(g_D3DDebugObjectName, 0, nullptr);
        }
#else
        ALIMER_UNUSED(obj);
        ALIMER_UNUSED(name);
#endif
    }
}
