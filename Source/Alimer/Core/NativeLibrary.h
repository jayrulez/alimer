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

#include "Core/Assert.h"
#include <string>

namespace Alimer
{
    class ALIMER_API NativeLibrary final 
    {
    public:
        NativeLibrary() = default;
        ~NativeLibrary();
        NativeLibrary(const NativeLibrary&) = delete;
        NativeLibrary& operator=(const NativeLibrary&) = delete;
        NativeLibrary(NativeLibrary&& other) noexcept;
        NativeLibrary& operator=(NativeLibrary&& other) noexcept;

        bool IsValid() const;
        bool Open(const std::string& filename, std::string* error = nullptr);
        void Close();

        void* GetProc(const std::string& procName, std::string* error = nullptr) const;

        template <typename T>
        bool GetProc(T** proc, const std::string& procName, std::string* error = nullptr) const {
            ALIMER_ASSERT(proc != nullptr);
            static_assert(std::is_function<T>::value, "");
            *proc = reinterpret_cast<T*>(GetProc(procName, error));
            return *proc != nullptr;
        }

    private:
        void* handle = nullptr;
    };
}
