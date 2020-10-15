// Copyright (c) 2019-2020 2019-2020 Amer Koleci and contributors.

#ifndef __ALIMER_COMMON__
#define __ALIMER_COMMON__

#if !defined(DXIL)
#define DXIL 0
#endif

#if DXIL || VULKAN
#define VERTEX_ATTRIBUTE(type, name, index) [[vk::location(index)]] type name : ATTRIBUTE##index
#else
#define VERTEX_ATTRIBUTE(type, name, index) type name : ATTRIBUTE##index
#endif

#endif
