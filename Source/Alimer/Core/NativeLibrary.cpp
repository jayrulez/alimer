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

#include "Core/NativeLibrary.h"
#include <algorithm>

#if ALIMER_PLATFORM_WINDOWS
#   include "foundation/windows.h"
#elif ALIMER_PLATFORM_POSIX
#   include <dlfcn.h>
#else
#   error "Unsupported platform for NativeLibrary"
#endif

namespace Alimer
{
    NativeLibrary::~NativeLibrary()
    {
        Close();
    }

    NativeLibrary::NativeLibrary(NativeLibrary&& other) noexcept
    {
        std::swap(handle, other.handle);
    }

    NativeLibrary& NativeLibrary::operator=(NativeLibrary&& other) noexcept
    {
        std::swap(handle, other.handle);
        return *this;
    }

    bool NativeLibrary::IsValid() const
    {
        return handle != nullptr;
    }

    bool NativeLibrary::Open(const std::string& filename, std::string* error)
    {
#if ALIMER_PLATFORM_WINDOWS
        handle = LoadLibraryA(filename.c_str());
        if (handle == nullptr && error != nullptr) {
            *error = "Windows Error: " + std::to_string(GetLastError());
        }
#elif ALIMER_PLATFORM_POSIX
        handle = dlopen(filename.c_str(), RTLD_NOW);
        if (handle == nullptr && error != nullptr) {
            *error = dlerror();
        }
#endif

        return handle != nullptr;
    }

    void NativeLibrary::Close()
    {
        if (handle == nullptr) {
            return;
        }
#if ALIMER_PLATFORM_WINDOWS
        FreeLibrary(static_cast<HMODULE>(handle));
#elif ALIMER_PLATFORM_POSIX
        dlclose(handle);
#endif
        handle = nullptr;
    }

    void* NativeLibrary::GetProc(const std::string& procName, std::string* error) const
    {
        void* proc = nullptr;
#if ALIMER_PLATFORM_WINDOWS
        proc = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), procName.c_str()));
        if (proc == nullptr && error != nullptr) {
            *error = "Windows Error: " + std::to_string(GetLastError());
        }
#elif ALIMER_PLATFORM_POSIX
        proc = reinterpret_cast<void*>(dlsym(handle, procName.c_str()));
        if (proc == nullptr && error != nullptr) {
            *error = dlerror();
        }
#endif
        return proc;
    }
}
