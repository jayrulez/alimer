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

#include "D3D11CommandContext.h"
#include "D3D11Buffer.h"
#include "D3D11Texture.h"
#include "D3D11GraphicsDevice.h"

namespace alimer
{
    D3D11CommandContext::D3D11CommandContext(D3D11GraphicsDevice* device, const ComPtr<ID3D11DeviceContext1>& context)
        : _device(device)
        , _context(context)
    {
    }

    void D3D11CommandContext::BindBuffer(uint32_t slot, Buffer* buffer)
    {
        if (buffer != nullptr)
        {
            auto d3d11Buffer = static_cast<D3D11Buffer*>(buffer)->GetHandle();
            _context->VSSetConstantBuffers(slot, 1, &d3d11Buffer);
            _context->PSSetConstantBuffers(slot, 1, &d3d11Buffer);
        }
        else
        {
            _context->VSSetConstantBuffers(slot, 1, nullptr);
            _context->PSSetConstantBuffers(slot, 1, nullptr);
        }
    }

    void D3D11CommandContext::BindBufferData(uint32_t slot, const void* data, uint32_t size)
    {
        // TODO:
    }
}
