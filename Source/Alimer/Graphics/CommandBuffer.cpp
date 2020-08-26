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

#include "Core/Log.h"
#include "Graphics/CommandBuffer.h"

namespace Alimer
{
    CommandBuffer::CommandBuffer(uint64_t memoryStreamBlockSize)
        : state(State::Initial)
        , blockSize(memoryStreamBlockSize)
    {
        if (blockSize)
        {
            AllocateNewBlock(4);
        }
    }

    void CommandBuffer::Commit()
    {
        CommitCore();
        state = State::Committed;
    }

    void CommandBuffer::WaitUntilCompleted()
    {
        WaitUntilCompletedCore();
    }

    void CommandBuffer::PushDebugGroup(const char* name)
    {
        Write(CommandID::PushDebugGroup);
        Write(name);
    }

    void CommandBuffer::PopDebugGroup()
    {
        Write(CommandID::PopDebugGroup);
    }

    void CommandBuffer::InsertDebugMarker(const char* name)
    {
        Write(CommandID::InsertDebugMarker);
        Write(name);
    }

    void CommandBuffer::BeginRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        Write(CommandID::BeginRenderPass);
        Write(numColorAttachments);
        if (numColorAttachments > 0)
            Write(colorAttachments, sizeof(RenderPassColorAttachment) * numColorAttachments);

        if (depthStencil != nullptr)
        {
            Write((uint8_t)1);
            Write(depthStencil, sizeof(RenderPassDepthStencilAttachment));
        }
        else
        {
            Write((uint8_t)0);
        }
    }

    void CommandBuffer::EndRenderPass()
    {
        Write(CommandID::EndRenderPass);
    }

    void CommandBuffer::AllocateNewBlock(uint64_t size)
    {
        if (blocks.size() == 0 || writeBlock->allocation.size() - writeBlock->writeAddr < size)
        {
            blocks.push_back({ std::vector<uint8_t>(blockSize), 0, 0 });
            writeBlock = blocks.end();
            --writeBlock;

            if (blocks.size() == 1)
                readBlock = blocks.begin();
        }
    }

    void CommandBuffer::ResetState()
    {
        // Reset the read and write positions in the memory stream.
        SeekP(0);
        SeekG(0);

        state = State::Initial;
    }

    uint64_t CommandBuffer::TellG()
    {
        if (blocks.size() == 0)
            return 0;

        if (readBlock == blocks.end())
            return blocks.size() * blockSize;

        return readBlock->readAddr + std::distance(blocks.begin(), readBlock) * blockSize;
    }

    uint64_t CommandBuffer::TellP()
    {
        if (blocks.size() == 0)
            return 0;

        return writeBlock->writeAddr + std::distance(blocks.begin(), writeBlock) * blockSize;
    }

    void CommandBuffer::SeekP(uint64_t pos)
    {
        auto blockIndex = static_cast<uint32_t>(floor(pos / blockSize));
        blockIndex = Min(blockIndex, static_cast<uint32_t>(blocks.size() - 1));
        writeBlock = blocks.begin();
        std::advance(writeBlock, blockIndex);
        writeBlock->writeAddr = pos % blockSize;
    }

    void CommandBuffer::SeekP(int64_t offset, SeekDir dir)
    {
        if (blocks.size() == 0)
            return;

        switch (dir)
        {
        case SeekDir::Begin:
        {
            SeekP(static_cast<uint64_t>(offset));
            break;
        }

        case SeekDir::Current:
        {
            if (offset > 0)
            {
                while (std::abs(offset) >= static_cast<int64_t>(blockSize) && writeBlock != blocks.end())
                {
                    offset -= blockSize;
                    ++writeBlock;
                }

                if (writeBlock == blocks.end())
                {
                    --writeBlock;
                    writeBlock->writeAddr = blockSize;
                }
                else
                {
                    writeBlock->writeAddr = offset;
                }
            }
            else if (offset < 0)
            {
                while (std::abs(offset) >= static_cast<int64_t>(blockSize) && writeBlock != blocks.begin())
                {
                    offset += blockSize;
                    --writeBlock;
                }

                if (offset >= static_cast<int64_t>(blockSize))
                {
                    writeBlock->writeAddr = 0;
                }
                else
                {
                    writeBlock->writeAddr -= offset;
                }
            }
            break;
        }

        case SeekDir::End:
            break;
        }
    }

    void CommandBuffer::SeekG(uint64_t pos)
    {
        auto blockIndex = static_cast<uint32_t>(floor(pos / blockSize));
        blockIndex = Min(blockIndex, static_cast<uint32_t>(blocks.size() - 1));
        readBlock = blocks.begin();
        std::advance(readBlock, blockIndex);
        readBlock->readAddr = pos % blockSize;
    }

    void CommandBuffer::SeekG(int64_t offset, SeekDir dir)
    {
        if (blocks.size() == 0)
            return;

        switch (dir)
        {
        case SeekDir::Begin:
        {
            SeekG(static_cast<uint64_t>(offset));
            break;
        }

        case SeekDir::Current:
        {
            if (offset > 0)
            {
                while (std::abs(offset) >= static_cast<int64_t>(blockSize) && readBlock != blocks.end())
                {
                    offset -= blockSize;
                    ++readBlock;
                }

                if (readBlock == blocks.end())
                {
                    --readBlock;
                    readBlock->readAddr = blockSize;
                }
                else
                {
                    readBlock->readAddr = offset;
                }
            }
            else if (offset < 0)
            {
                while (std::abs(offset) >= static_cast<int64_t>(blockSize) && readBlock != blocks.begin())
                {
                    offset -= blockSize;
                    --readBlock;
                }

                if (offset >= static_cast<int64_t>(blockSize))
                {
                    readBlock->readAddr = 0;
                }
                else
                {
                    readBlock->readAddr = offset;
                }
            }
            break;
        }

        case SeekDir::End:
            break;
        }
    }

    bool CommandBuffer::EndOfStream()
    {
        return (blocks.size() == 0 || readBlock == blocks.end());
    }

    void CommandBuffer::Write(CommandID command)
    {
        Write<uint8_t>(static_cast<uint8_t>(command));
    }

    void CommandBuffer::Write(const char* str)
    {
        const uint32_t length = static_cast<uint32_t>(strlen(str));
        Write(length + 1);
        Write(str, length + 1);
    }

    void CommandBuffer::Write(const void* pData, uint64_t size)
    {
        if (size == 0)
            return;

        AllocateNewBlock(size);

        if (writeBlock->writeAddr + size > writeBlock->allocation.size())
        {
            ++writeBlock;
            writeBlock->writeAddr = 0;
        }

        memcpy(&writeBlock->allocation[writeBlock->writeAddr], pData, size);
        writeBlock->writeAddr += size;
    }

    CommandBuffer::CommandID CommandBuffer::ReadCommandID()
    {
        return (CommandBuffer::CommandID)Read<uint8_t>();
    }

    void CommandBuffer::Read(void* pData, uint64_t size)
    {
        if (size == 0)
            return;

        if (blocks.size() == 0)
            return;

        if (readBlock->readAddr + size > readBlock->writeAddr)
        {
            if (++readBlock == blocks.end())
                return;
            else
                readBlock->readAddr = 0;
        }

        memcpy(pData, &readBlock->allocation[readBlock->readAddr], size);
        readBlock->readAddr += size;
    }

    std::string CommandBuffer::ReadString()
    {
        const uint32_t length = Read<uint32_t>();
        const char* str = ReadPtr<char>(length);
        return std::string(str, length);
    }
}

