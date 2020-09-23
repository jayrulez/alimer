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

#include <string>
#include <functional>

namespace ShaderCompiler
{
    enum class ShaderStage : uint32_t
    {
        Vertex,
        Pixel,
        Compute,
        Count
    };

    enum class SourceShaderLanguage : uint32_t
    {
        HLSL,
        LegacyHLSL
    };

    enum class ShadingLanguage : uint32_t
    {
        DXIL = 0,
        SPIRV,
        HLSL,

        Count
    };

    struct SourceDesc
    {
        const char* source;
        const char* fileName;
        const char* entryPoint;
        ShaderStage stage;
        SourceShaderLanguage language;
    };

    struct TargetDesc
    {
        ShadingLanguage language = ShadingLanguage::DXIL;
    };

    bool Compile(const SourceDesc& source, const TargetDesc& target);
}
