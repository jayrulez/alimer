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

    enum class ShadingLanguage : uint32_t
    {
        DXIL = 0,
        SPIRV,
        HLSL,

        Count
    };

    class Blob
    {
    public:
        Blob() noexcept = default;
        Blob(const void* data, uint32_t size);
        Blob(const Blob& other);
        Blob(Blob&& other) noexcept;
        ~Blob() noexcept;

        Blob& operator=(const Blob& other);
        Blob& operator=(Blob&& other) noexcept;

        void Reset();
        void Reset(const void* newData, uint32_t size);

        const void* Data() const noexcept;
        uint32_t Size() const noexcept;

    private:
        std::vector<uint8_t> data;
    };

    struct ShaderModel
    {
        uint8_t major_ver : 6;
        uint8_t minor_ver : 2;

        uint32_t FullVersion() const noexcept
        {
            return (major_ver << 2) | minor_ver;
        }

        bool operator<(const ShaderModel& other) const noexcept
        {
            return this->FullVersion() < other.FullVersion();
        }
        bool operator==(const ShaderModel& other) const noexcept
        {
            return this->FullVersion() == other.FullVersion();
        }
        bool operator>(const ShaderModel& other) const noexcept
        {
            return other < *this;
        }
        bool operator<=(const ShaderModel& other) const noexcept
        {
            return (*this < other) || (*this == other);
        }
        bool operator>=(const ShaderModel& other) const noexcept
        {
            return (*this > other) || (*this == other);
        }
    };

    struct CompileOptions
    {
        ShaderModel shaderModel = { 6, 0 };
    };

    struct SourceDesc
    {
        const char* source;
        const char* fileName;
    };

    struct TargetDesc
    {
        ShadingLanguage language = ShadingLanguage::HLSL;
    };

    struct Shader
    {
        ShaderStage stage;
        Blob bytecode;
    };

    struct ResultDesc
    {
        bool hasError;
        Blob errors;

        Blob output;
        std::vector<Shader> shaders;
    };

    ResultDesc Compile(const SourceDesc& source, const CompileOptions& options, const TargetDesc& target);
}
