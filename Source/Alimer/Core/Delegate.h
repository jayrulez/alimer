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

#include "Core/Platform.h"
#include <EASTL/vector.h>

namespace alimer
{
    template <typename T> struct Function;
    template <typename T> struct Delegate;

    template <typename R, typename... Args> struct Function<R(Args...)>
    {
    private:
        using InstancePtr = void*;
        using InternalFunction = R(*)(InstancePtr, Args...);

        struct Stub
        {
            InstancePtr first;
            InternalFunction second;
        };

        template <R(*Function)(Args...)> static R FunctionStub(InstancePtr, Args... args)
        {
            return (Function)(args...);
        }

        template <typename C, R(C::* Function)(Args...)>
        static R ClassMethodStub(InstancePtr instance, Args... args)
        {
            return (static_cast<C*>(instance)->*Function)(args...);
        }

        template <typename C, R(C::* Function)(Args...) const>
        static R ClassMethodStub(InstancePtr instance, Args... args)
        {
            return (static_cast<C*>(instance)->*Function)(args...);
        }

    public:
        Function()
        {
            stub_.first = nullptr;
            stub_.second = nullptr;
        }

        bool IsValid() { return stub_.second != nullptr; }

        template <R(*F)(Args...)> void Bind()
        {
            stub_.first = nullptr;
            stub_.second = &FunctionStub<F>;
        }

        template <auto F, typename C> void Bind(C* instance)
        {
            stub_.first = instance;
            stub_.second = &ClassMethodStub<C, F>;
        }

        R Invoke(Args... args) const
        {
            ALIMER_ASSERT(stub_.second != nullptr);
            return stub_.second(stub_.first, args...);
        }

        bool operator==(const Delegate<R(Args...)>& rhs)
        {
            return stub_.first == rhs.stub_.first && stub_.second == rhs.stub_.second;
        }

    private:
        Stub stub_;
    };

    template <typename R, typename... Args> struct Delegate<R(Args...)>
    {
    public:
        Delegate() noexcept = default;

        template <auto F, typename C> void Bind(C* instance)
        {
            Function<R(Args...)> cb;
            cb.template Bind<F>(instance);
            delegates.push(cb);
        }

        template <R(*F)(Args...)> void Bind()
        {
            Function<R(Args...)> cb;
            cb.template Bind<F>();
            delegates.push(cb);
        }

        template <R(*F)(Args...)> void Unbind()
        {
            Function<R(Args...)> cb;
            cb.template Bind<F>();
            for (uint32_t i = 0; i < delegates.size(); ++i)
            {
                if (delegates[i] == cb)
                {
                    delegates.EraseSwap(i);
                    break;
                }
            }
        }

        template <auto F, typename C> void Unbind(C* instance)
        {
            Function<R(Args...)> cb;
            cb.template Bind<F>(instance);
            for (uint32_t i = 0; i < delegates.size(); ++i)
            {
                if (delegates[i] == cb)
                {
                    delegates.EraseSwap(i);
                    break;
                }
            }
        }

        void Invoke(Args... args)
        {
            for (uint32_t i = 0, count = delegates.size(); i < count; ++i) {
                delegates[i].Invoke(args...);
            }
        }

    private:
        eastl::vector<Function<R(Args...)>> delegates;
    };
}
