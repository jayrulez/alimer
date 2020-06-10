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

#include "Core/Library.h"
#if defined(_DEBUG)
#include "Core/Log.h"
#endif

namespace alimer
{
    LibHandle LibraryOpen(const char* libName)
    {
        HMODULE handle = LoadLibraryA(libName);

#if defined(_DEBUG)
        if (handle == nullptr)
        {
            LOG_WARN("LibraryOpen - Windows Error: %d", GetLastError());
        }
#endif

        return (LibHandle)handle;
    }

    void LibraryClose(LibHandle handle)
    {
        FreeLibrary(static_cast<HMODULE>(handle));
    }

    void* LibrarySymbol(LibHandle handle, const char* symbolName)
    {
        void* proc = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), symbolName));

#if defined(_DEBUG)
        if (proc == nullptr)
        {
            LOG_WARN("LibrarySymbol - Windows Error: %d", GetLastError());
        }
#endif
        return proc;
    }
}
