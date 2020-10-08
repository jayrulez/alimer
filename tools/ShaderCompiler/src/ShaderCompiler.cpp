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

#include "ShaderCompiler.h"
#include "MemoryStream.h"
#include <assert.h>

#if defined(_WIN32) || defined(_WIN64)
#   include <d3dcompiler.h>
#endif

namespace ShaderCompiler
{
    namespace
    {
#if defined(_WIN32) || defined(_WIN64)
        static HINSTANCE d3dcompiler_dll = nullptr;
        static bool d3dcompiler_dll_load_failed = false;
        static pD3DCompile D3DCompile_func;

        inline bool LoadD3DCompilerDLL(void)
        {
            /* load DLL on demand */
            if (d3dcompiler_dll == nullptr && !d3dcompiler_dll_load_failed)
            {
                d3dcompiler_dll = LoadLibraryW(L"d3dcompiler_47.dll");
                if (d3dcompiler_dll == nullptr)
                {
                    /* don't attempt to load missing DLL in the future */
                    d3dcompiler_dll_load_failed = true;
                    return false;
                }

                /* look up function pointers */
                D3DCompile_func = (pD3DCompile)GetProcAddress(d3dcompiler_dll, "D3DCompile");
                assert(D3DCompile_func != nullptr);
            }

            return d3dcompiler_dll != nullptr;
        }

        class D3DLegacyIncludeHandler : public ID3DInclude
        {
        public:
        };

        std::string LegacyShaderProfileName(ShaderStage stage, ShaderModel shaderModel)
        {
            std::string shaderProfile;
            switch (stage)
            {
            case ShaderStage::Vertex:
                shaderProfile = "vs";
                break;

            case ShaderStage::Pixel:
                shaderProfile = "ps";
                break;

            case ShaderStage::Compute:
                shaderProfile = "cs";
                break;

            default:
                assert(false && "Invalid shader stage.");
            }

            shaderProfile.push_back('_');
            shaderProfile.push_back('0' + shaderModel.major_ver);
            shaderProfile.push_back('_');
            shaderProfile.push_back('0' + shaderModel.minor_ver);

            return shaderProfile;
        }

        std::string EntryPoint(ShaderStage stage)
        {
            std::string shaderProfile;
            switch (stage)
            {
            case ShaderStage::Vertex:
                return "VSMain";

            case ShaderStage::Pixel:
                return "PSMain";

            case ShaderStage::Compute:
                return "CSMain";

            default:
                assert(false && "Invalid shader stage.");
            }

            return "";
        }

        inline bool CompileLegacy(const SourceDesc& source, ShaderStage stage, ShaderModel shaderModel, Blob* result, Blob* errors)
        {
            ID3DBlob* output = nullptr;
            ID3DBlob* errors_or_warnings = nullptr;

            std::string profileName = LegacyShaderProfileName(stage, shaderModel);
            std::string entryPoint = EntryPoint(stage);

            HRESULT hr = D3DCompile_func(
                source.source,
                strlen(source.source),
                source.fileName,
                NULL,                           
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
                entryPoint.c_str(),
                profileName.c_str(),
                D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3, 
                0,          
                &output,    
                &errors_or_warnings);

            if (FAILED(hr)) {
                const char* errorMsg = (LPCSTR)errors_or_warnings->GetBufferPointer();
                errors->Reset(errors_or_warnings->GetBufferPointer(), static_cast<uint32_t>(errors_or_warnings->GetBufferSize()));
                return false;
            }

            result->Reset(output->GetBufferPointer(), static_cast<uint32_t>(output->GetBufferSize()));
            return true;
        }
#endif
        std::wstring ShaderProfileName(ShaderStage stage, ShaderModel shaderModel)
        {
            std::wstring shaderProfile;
            switch (stage)
            {
            case ShaderStage::Vertex:
                shaderProfile = L"vs";
                break;

            case ShaderStage::Pixel:
                shaderProfile = L"ps";
                break;

            case ShaderStage::Compute:
                shaderProfile = L"cs";
                break;

            default:
                assert(false && "Invalid shader stage.");
            }

            shaderProfile.push_back(L'_');
            shaderProfile.push_back(L'0' + shaderModel.major_ver);
            shaderProfile.push_back(L'_');
            shaderProfile.push_back(L'0' + shaderModel.minor_ver);

            return shaderProfile;
        }
    }

    Blob::Blob(const void* data, uint32_t size)
    {
        Reset(data, size);
    }

    Blob::Blob(const Blob& other)
    {
        Reset(other.Data(), other.Size());
    }

    Blob::Blob(Blob&& other) noexcept
        : data(std::move(other.data))
    {
        other.data.clear();
    }

    Blob::~Blob() noexcept
    {
        data.clear();
    }

    Blob& Blob::operator=(const Blob& other)
    {
        if (this != &other)
        {
            this->Reset(other.Data(), other.Size());
        }
        return *this;
    }

    Blob& Blob::operator=(Blob&& other) noexcept
    {
        if (this != &other)
        {
            data = std::move(other.data);
            other.data.clear();
        }

        return *this;
    }

    void Blob::Reset()
    {
        data.clear();
    }

    void Blob::Reset(const void* newData, uint32_t size)
    {
        this->Reset();
        if ((newData != nullptr) && (size > 0))
        {
            data = std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(newData), reinterpret_cast<const uint8_t*>(newData) + size);
        }
    }

    const void* Blob::Data() const noexcept
    {
        return data.data();
    }

    uint32_t Blob::Size() const noexcept
    {
        return static_cast<uint32_t>(data.size());
    }

    ResultDesc Compile(const SourceDesc& source, const CompileOptions& options, const TargetDesc& target)
    {
        ResultDesc result{};
        result.hasError = true;

        if (options.shaderModel.major_ver <= 5)
        {
#if defined(_WIN32) || defined(_WIN64)
            if (!LoadD3DCompilerDLL())
            {
                return result;
            }

            Shader vertexShader;
            Blob fragmentShader;
            Blob errorMessage;

            if (!CompileLegacy(source, ShaderStage::Vertex, options.shaderModel, &vertexShader.bytecode, &errorMessage))
            {
                return result;
            }

            result.shaders.push_back(vertexShader);

            if (!CompileLegacy(source, ShaderStage::Pixel, options.shaderModel, &fragmentShader, &errorMessage))
            {
                return result;
            }
#endif
        }
        else
        {

        }
        result.hasError = false;
        return result;
    }
}
