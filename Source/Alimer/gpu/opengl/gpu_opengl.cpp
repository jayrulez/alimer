#if defined(GPU_DRIVER_OPENGL)
#include "gpu/gpu_backend.h"

#if defined(__EMSCRIPTEN__)
#   define GPU_WEBGL
#   include <GLES3/gl3.h>
#   include <GLES2/gl2ext.h>
#   include <GL/gl.h>
#   include <GL/glext.h>
#elif defined(__ANDROID__)
#   define GPU_GLES
#   include <glad/glad.h>
#else
#   define GPU_GL
#   include "glad.h"
#endif

#endif /* defined(GPU_DRIVER_OPENGL) */
