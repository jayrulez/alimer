#ifndef __RHI_INTERNAL_H
#define __RHI_INTERNAL_H

//  Descriptor layout counts per shader stage:
//  Rebuilding shaders and graphics devices are necessary after changing these values
#define GPU_RESOURCE_HEAP_CBV_COUNT		12
#define GPU_RESOURCE_HEAP_SRV_COUNT		48
#define GPU_RESOURCE_HEAP_UAV_COUNT		8
#define GPU_SAMPLER_HEAP_COUNT			16

#ifdef __cplusplus
#include <atomic>

namespace Alimer
{
    class SpinLock
    {
    private:
        std::atomic_flag lck = ATOMIC_FLAG_INIT;
    public:
        void lock()
        {
            while (!try_lock()) {}
        }
        bool try_lock()
        {
            return !lck.test_and_set(std::memory_order_acquire);
        }

        void unlock()
        {
            lck.clear(std::memory_order_release);
        }
    };

    // Fixed size very simple thread safe ring buffer
    template <typename T, size_t capacity>
    class ThreadSafeRingBuffer
    {
    public:
        // Push an item to the end if there is free space
        //	Returns true if succesful
        //	Returns false if there is not enough space
        inline bool push_back(const T& item)
        {
            bool result = false;
            lock.lock();
            size_t next = (head + 1) % capacity;
            if (next != tail)
            {
                data[head] = item;
                head = next;
                result = true;
            }
            lock.unlock();
            return result;
        }

        // Get an item if there are any
        //	Returns true if succesful
        //	Returns false if there are no items
        inline bool pop_front(T& item)
        {
            bool result = false;
            lock.lock();
            if (tail != head)
            {
                item = data[tail];
                tail = (tail + 1) % capacity;
                result = true;
            }
            lock.unlock();
            return result;
        }

    private:
        T data[capacity];
        size_t head = 0;
        size_t tail = 0;
        SpinLock lock;
    };
}

#endif  // __cplusplus

#endif // __RHI_INTERNAL_H
