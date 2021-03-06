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

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define CXXOPTS_NO_RTTI
#include <cxxopts.hpp>

int main(int argc, const char* argv[])
{
    cxxopts::Options options("Alimer ShaderCompiler", "A tool for compiling HLSL.");
    // clang-format off
    options.add_options()
        ("E,entry", "Entry point of the shader", cxxopts::value<std::string>()->default_value("main"))
        ("I,input", "Input file name", cxxopts::value<std::string>())("O,output", "Output file name", cxxopts::value<std::string>())
        ("T,target", "Target shading language: dxil, spirv, hlsl, glsl, essl, msl_macos, msl_ios", cxxopts::value<std::string>()->default_value("dxil"))
        ("V,version", "The version of target shading language", cxxopts::value<std::string>()->default_value(""))
        ("D,define", "Macro define as name=value", cxxopts::value<std::vector<std::string>>());

    auto opts = options.parse(argc, argv);

    if ((opts.count("input") == 0))
    {
        std::cerr << "COULDN'T find <input> or <stage> in command line parameters." << std::endl;
        std::cerr << options.help() << std::endl;
        return 1;
    }

    using namespace ShaderCompiler;
    SourceDesc sourceDesc{};
    TargetDesc targetDesc{};

    const auto fileName = opts["input"].as<std::string>();
    const auto targetName = opts["target"].as<std::string>();
    const auto targetVersion = opts["version"].as<std::string>();

    sourceDesc.fileName = fileName.c_str();

    std::string outputName;
    if (opts.count("output") == 0)
    {
        static const std::string extMap[] = { "dxil", "spv", "hlsl"/*, "glsl", "essl", "msl", "msl"*/ };
        static_assert(sizeof(extMap) / sizeof(extMap[0]) == static_cast<uint32_t>(ShadingLanguage::Count),
            "extMap doesn't match with the number of shading languages.");
        outputName = fileName + "." + extMap[static_cast<uint32_t>(targetDesc.language)];
        outputName = fileName + "." + "cso";
    }
    else
    {
        outputName = opts["output"].as<std::string>();
    }

    // Read input source.
    std::string source;
    {
        std::ifstream inputFile(fileName, std::ios_base::binary);
        if (!inputFile)
        {
            std::cerr << "COULDN'T load the input file: " << fileName << std::endl;
            return 1;
        }

        inputFile.seekg(0, std::ios::end);
        source.resize(static_cast<size_t>(inputFile.tellg()));
        inputFile.seekg(0, std::ios::beg);
        inputFile.read(&source[0], source.size());
    }
    sourceDesc.source = source.c_str();

    CompileOptions compileOptions{};
    compileOptions.shaderModel = { 5, 0 };

    ResultDesc result = Compile(sourceDesc, compileOptions, targetDesc);

    if (result.hasError && result.errors.Size() > 0)
    {
        const char* msg = reinterpret_cast<const char*>(result.errors.Data());
        std::cerr << "Error or warning from shader compiler: " << std::endl
            << std::string(msg, msg + result.errors.Size()) << std::endl;
    }

    {
        FileStream stream(outputName);
        stream.WriteFileID("ASHD");
        stream.Write(static_cast<uint32_t>(result.shaders.size()));
    }

    if (result.output.Size() > 0)
    {
        std::ofstream outputFile(outputName, std::ios_base::binary);
        if (!outputFile)
        {
            std::cerr << "COULDN'T open the output file: " << outputName << std::endl;
            return 1;
        }

        outputFile.write(reinterpret_cast<const char*>(result.output.Data()), result.output.Size());

        std::cout << "The compiled file is saved to " << outputName << std::endl;
    }

    return result.hasError ? EXIT_FAILURE : EXIT_SUCCESS;
}

