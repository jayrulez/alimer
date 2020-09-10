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

#include "Graphics/GPUBuffer.h"
#include "Graphics/Texture.h"
#include "Math/Size.h"
#include "Math/Color.h"
#include "Math/Viewport.h"
#include <vector>
#include <list>

namespace Alimer
{
    /// A container that stores commands for the GPU to execute.
    class ALIMER_API CommandBuffer : public RefCounted
    {
    public:
        enum class State
        {
            Initial,
            Enqueued,
            Committed,
            Scheduled,
            Completed
        };

    protected:
        /// Constructor.
        CommandBuffer(uint64_t memoryStreamBlockSize);

    public:
        /// Destructor.
        virtual ~CommandBuffer() = default;

        //virtual void BindBuffer(uint32_t slot, GPUBuffer* buffer) = 0;
        //virtual void BindBufferData(uint32_t slot, const void* data, uint32_t size) = 0;

    protected:
        void ResetState();

    private:

        State state;

        /* Command stream */
        struct MemoryBlock
        {
            std::vector<uint8_t> allocation;
            uint64_t readAddr;
            uint64_t writeAddr;
        };

        void AllocateNewBlock(uint64_t size);

        uint64_t blockSize;
        std::list<MemoryBlock> blocks;
        std::list<MemoryBlock>::iterator readBlock;
        std::list<MemoryBlock>::iterator writeBlock;

    protected:
        enum class CommandID : uint8_t
        {
            PushDebugGroup,
            PopDebugGroup,
            InsertDebugMarker,
            BeginRenderPass,
            EndRenderPass,
            Count
        };

        enum class SeekDir
        {
            Begin,
            End,
            Current
        };

        uint64_t TellG();
        uint64_t TellP();
        void SeekP(uint64_t pos);
        void SeekP(int64_t offset, SeekDir dir);
        void SeekG(uint64_t pos);
        void SeekG(int64_t offset, SeekDir dir);
        bool EndOfStream();

        template <class T> void Write(const T& value)
        {
            Write(reinterpret_cast<const void*>(&value), sizeof(value));
        }

        void Write(CommandID command);
        void Write(const char* str);
        void Write(const void* pData, uint64_t size);

        template <class T> T Read()
        {
            T ret;
            Read(&ret, sizeof ret);
            return ret;
        }

        template <typename T>
        T* ReadPtr(uint64_t count = 1)
        {
            if (blocks.size() == 0)
                return nullptr;

            if (readBlock->readAddr + sizeof(T) * count > readBlock->writeAddr)
            {
                if (++readBlock == blocks.end())
                    return nullptr;
                else
                    readBlock->readAddr = 0;
            }

            auto ptr = reinterpret_cast<T*>(&readBlock->allocation[readBlock->readAddr]);
            readBlock->readAddr += sizeof(T) * count;
            return ptr;
        }


        CommandID ReadCommandID();
        void Read(void* pData, uint64_t size);
        std::string ReadString();

    protected:
    };
}
