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

#ifndef _VGPU_DRIVER_H
#define _VGPU_DRIVER_H

#include "vgpu.h"

#ifndef VGPU_MALLOC
#   include <stdlib.h>
#   define VGPU_MALLOC(s) malloc(s)
#   define VGPU_FREE(p) free(p)
#endif

#ifndef VGPU_ALLOCA
#   include <malloc.h>
#   if defined(_MSC_VER) || defined(__MINGW32__)
#       define VGPU_ALLOCA(type, count) ((type*)(_malloca(sizeof(type) * (count))))
#   else
#       define VGPU_ALLOCA(type, count) ((type*)(alloca(sizeof(type) * (count))))
#   endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#   if defined(__i386__) || defined(__x86_64__)
#       define VGPU_BREAKPOINT() __asm__ __volatile__("int $3\n\t")
#   else
#       define VGPU_BREAKPOINT() ((void)0)
#   endif
#   define VGPU_UNREACHABLE() __builtin_unreachable()

#elif defined(_MSC_VER)
extern void __cdecl __debugbreak(void);
#   define VGPU_BREAKPOINT() __debugbreak()
#   define VGPU_UNREACHABLE() __assume(false)
#else
#   error "Unsupported compiler"
#endif

#define VGPU_UNUSED(x) do { (void)sizeof(x); } while(0)

#define _vgpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _vgpu_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _vgpu_min(a,b) ((a<b)?a:b)
#define _vgpu_max(a,b) ((a>b)?a:b)
#define _vgpu_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define _vgpu_count_of(x) (sizeof(x) / sizeof(x[0]))
#define _vgpu_align_to(_alignment, _val) ((((_val) + (_alignment) - 1) / (_alignment)) * (_alignment))

#include <new>
#include <vector>

enum {
    _VGPU_SLOT_SHIFT = 16,
    _VGPU_SLOT_MASK = (1 << _VGPU_SLOT_SHIFT) - 1,
    _VGPU_MAX_POOL_SIZE = (1 << _VGPU_SLOT_SHIFT),
};

/* this *MUST* remain 0 */
#define _VGPU_INVALID_SLOT_INDEX (0)

typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _vgpu_pool_t;

typedef struct {
    uint32_t id;
    uint32_t ctx_id;
    //sg_resource_state state;
} _vgpu_slot_t;

static void _vgpu_init_pool(_vgpu_pool_t* pool, int num) {
    VGPU_ASSERT(pool && (num >= 1));
    /* slot 0 is reserved for the 'invalid id', so bump the pool size by 1 */
    pool->size = num + 1;
    pool->queue_top = 0;
    /* generation counters indexable by pool slot index, slot 0 is reserved */
    size_t gen_ctrs_size = sizeof(uint32_t) * pool->size;
    pool->gen_ctrs = (uint32_t*)VGPU_MALLOC(gen_ctrs_size);
    VGPU_ASSERT(pool->gen_ctrs);
    memset(pool->gen_ctrs, 0, gen_ctrs_size);
    /* it's not a bug to only reserve 'num' here */
    pool->free_queue = (int*)VGPU_MALLOC(sizeof(int) * num);
    VGPU_ASSERT(pool->free_queue);
    /* never allocate the zero-th pool item since the invalid id is 0 */
    for (int i = pool->size - 1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

static int _vgpu_pool_alloc_index(_vgpu_pool_t* pool) {
    VGPU_ASSERT(pool);
    VGPU_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        VGPU_ASSERT((slot_index > 0) && (slot_index < pool->size));
        return slot_index;
    }
    else {
        /* pool exhausted */
        return _VGPU_INVALID_SLOT_INDEX;
    }
}

/* allocate the slot at slot_index:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the resource id
*/
static uint32_t _vgpu_slot_alloc(_vgpu_pool_t* pool, _vgpu_slot_t* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    VGPU_ASSERT(pool && pool->gen_ctrs);
    VGPU_ASSERT((slot_index > _VGPU_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    //VGPU_ASSERT((slot->state == SG_RESOURCESTATE_INITIAL) && (slot->id == VGPU_INVALID_ID));
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr << _VGPU_SLOT_SHIFT) | (slot_index & _VGPU_SLOT_MASK);
    //slot->state = SG_RESOURCESTATE_ALLOC;
    return slot->id;
}

static int _vgpu_slot_index(uint32_t id) {
    int slot_index = (int)(id & _VGPU_SLOT_MASK);
    VGPU_ASSERT(slot_index != _VGPU_INVALID_SLOT_INDEX);
    return slot_index;
}

template <typename T, uint32_t MAX_COUNT>
struct Pool
{
    void init()
    {
        values = (T*)mem;
        for (int i = 1; i < MAX_COUNT + 1; ++i) {
            new (&values[i]) int(i + 1);
        }
        new (&values[MAX_COUNT - 1]) int(-1);
        first_free = 0;
    }

    int alloc()
    {
        if (first_free == -1) return -1;

        const int id = first_free;
        first_free = *((int*)&values[id]);
        new (&values[id]) T;
        return id;
    }

    void dealloc(uint32_t index)
    {
        values[index].~T();
        new (&values[index]) int(first_free);
        first_free = index;
    }

    alignas(T) uint8_t mem[(MAX_COUNT + 1) * sizeof(T)];
    T* values;
    int first_free;

    T& operator[](int index) { return values[index]; }
    bool isFull() const { return first_free == -1; }
};

struct Renderer
{
    bool (*init)(const vgpu_config* config);
    void (*shutdown)(void);
    void (*beginFrame)(void);
    void (*endFrame)(void);
    const vgpu::Caps* (*queryCaps)(void);

    /* Resource creation methods */
    vgpu_texture(*createTexture)(const vgpu::TextureDescription& desc, const void* initialData);
    void(*destroyTexture)(vgpu_texture handle);

    vgpu::BufferHandle(*createBuffer)(uint32_t size, vgpu::BufferUsage usage, uint32_t stride, const void* initialData);
    void (*destroyBuffer)(vgpu::BufferHandle handle);
    void* (*MapBuffer)(vgpu::BufferHandle handle);
    void (*UnmapBuffer)(vgpu::BufferHandle handle);

    vgpu::ShaderBlob(*compileShader)(const char* source, const char* entryPoint, vgpu::ShaderStage stage);
    vgpu::ShaderHandle(*createShader)(const vgpu::ShaderDescription& desc);
    void (*destroyShader)(vgpu::ShaderHandle handle);

    /* Commands */
    void (*cmdSetViewport)(float x, float y, float width, float height, float min_depth, float max_depth);
    void (*cmdSetScissor)(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void (*cmdSetVertexBuffer)(vgpu::BufferHandle handle);
    void (*cmdSetIndexBuffer)(vgpu::BufferHandle handle);
    void (*cmdSetShader)(vgpu::ShaderHandle handle);
    void (*cmdBindUniformBuffer)(uint32_t slot, vgpu::BufferHandle handle);
    void (*cmdBindTexture)(uint32_t slot, vgpu_texture handle);
    void (*cmdDrawIndexed)(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex);
};

struct Driver {
    vgpu::BackendType backendType;
    bool(*isSupported)(void);
    Renderer* (*initRenderer)(void);
};

extern Driver d3d11_driver;
extern Driver vulkan_driver;

#endif /* _VGPU_DRIVER_H */
