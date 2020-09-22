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

#pragma once

#include "Core/Object.h"

namespace Alimer
{
    class AssetLoader;

    class ALIMER_API AssetManager : public Object
    {
        ALIMER_OBJECT(AssetManager, Object);

    public:
        /// Constructor.
        AssetManager(const std::string& rootDirectory);
        /// Destructor.
        ~AssetManager();

        void AddLoader(std::unique_ptr<AssetLoader> loader);
        void RemoveLoader(const AssetLoader* loader);
        AssetLoader* GetLoader(StringId32 type);

        /// Load content from name.
        RefPtr<Object> Load(StringId32 type, const std::string& name);

        /// Load content from name, template version.
        template <class T> RefPtr<T> Load(const std::string& name) { return StaticCast<T>(Load(T::GetTypeStatic(), name)); }

    protected:
        std::string rootDirectory;

        std::unordered_map<StringId32, std::unique_ptr<AssetLoader>> loaders;
    };
}
