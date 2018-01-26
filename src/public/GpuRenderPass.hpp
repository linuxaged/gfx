#pragma once

#include "Gfx.hpp"
#include "GpuContext.hpp"

namespace lxd
{
	static const int EXPLICIT_RESOLVE = 0;

typedef enum
{
    GPU_RENDERPASS_TYPE_INLINE,
    GPU_RENDERPASS_TYPE_SECONDARY_COMMAND_BUFFERS
} GpuRenderPassType;

typedef enum
{
    GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER = 0b0,
    GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER = 0b1
} GpuRenderPassFlags;

class GpuContext;

class GpuRenderPass
{
  public:
    GpuRenderPass( GpuContext* context, const GpuSurfaceColorFormat colorFormat,
                   const GpuSurfaceDepthFormat depthFormat, const GpuSampleCount sampleCount,
                   const GpuRenderPassType type, const int flags );
    ~GpuRenderPass();

    GpuContext&           context;
    GpuRenderPassType     type;
    int                   flags;
    GpuSurfaceColorFormat colorFormat;
    GpuSurfaceDepthFormat depthFormat;
    GpuSampleCount        sampleCount;
    VkFormat              internalColorFormat;
    VkFormat              internalDepthFormat;
    VkRenderPass          renderPass;
};
} // namespace lxd
