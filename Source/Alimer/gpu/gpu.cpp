#include "gpu_backend.h"
#include <stdarg.h>
#include <stdio.h>

static GPULogCallback s_log_function = NULL;
static void* s_log_userData = NULL;

void gpuSetLogCallback(GPULogCallback callback, void* userData) {
    s_log_function = callback;
    s_log_userData = userData;
}

void gpuLog(GPULogLevel level, const char* format, ...) {
    if (s_log_function) {
        char message[GPU_MAX_LOG_MESSAGE_LENGTH];
        va_list args;
        va_start(args, format);
        vsnprintf(message, sizeof(message), format, args);
        s_log_function(s_log_userData, level, message);
        va_end(args);
    }
}

GPUDevice* gpuDeviceCreate(GPUBackendType backendType, bool debug, const GPUSwapChainDescriptor* descriptor)
{
    /*if (backendType == GPUBackendType_Force32)
    {
        for (uint32_t i = 0; i < GPU_COUNTOF(drivers); i++) {
            if (drivers[i]->IsSupported()) {
                return drivers[i]->CreateDevice(debug, descriptor);
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < GPU_COUNTOF(drivers); i++)
        {
            if (drivers[i]->backendType == backendType && drivers[i]->IsSupported()) {
                return drivers[i]->CreateDevice(debug, descriptor);
            }
        }
    }*/

    return NULL;
}

void gpuDeviceDestroy(GPUDevice* device) {
    if (device == NULL)
    {
        return;
    }

    device->Destroy(device);
}

void gpuBeginFrame(GPUDevice* device) {
    device->BeginFrame(device->driverData);
}

void gpuEndFrame(GPUDevice* device) {
    device->EndFrame(device->driverData);
}

gpu_features gpuDeviceGetFeatures(GPUDevice* device) {
    return device->GetFeatures(device->driverData);
}

gpu_limits gpuDeviceGetLimits(GPUDevice* device) {
    return device->GetLimits(device->driverData);
}

GPUContext* gpuDeviceCreateContext(GPUDevice* device, const GPUSwapChainDescriptor* descriptor)
{
    GPU_ASSERT(device);
    GPU_ASSERT(descriptor);

    GPUContext* context = (GPUContext*)GPU_MALLOC(sizeof(GPUContext));
    context->device = device;
    context->backend = device->CreateContext(device->driverData, descriptor);
    return context;
}

void gpuDeviceDestroyContext(GPUDevice* device, GPUContext* context) {
    device->DestroyContext(device->driverData, context->backend);
    GPU_FREE(device);
}

bool gpuContextResize(GPUContext* context, uint32_t width, uint32_t height) {
    return context->device->ResizeContext(context->device->driverData, context->backend, width, height);
}

#include "gpu/gpu_backend.h"

namespace Alimer
{
    namespace gpu
    {
        static Renderer* s_renderer = nullptr;
        static LogCallback s_logCallback = nullptr;
        static void* s_logCallbackUserData = nullptr;

        bool initialize(const Configuration& config, IAllocator& allocator)
        {
            if (s_renderer != nullptr) {
                return true;
            }

            s_logCallback = config.logCallback;
            s_logCallbackUserData = config.userData;

            BackendType backendType = config.backendType;
            if (config.backendType == BackendType::Count)
            {
                backendType = BackendType::Vulkan;
            }

            return true;
        }

        void shutdown()
        {
        }
    }
}
