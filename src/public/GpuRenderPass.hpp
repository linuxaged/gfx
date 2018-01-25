#pragma once
#include "Gfx.hpp"
namespace lxd
{
typedef enum
{
    GPU_RENDERPASS_TYPE_INLINE,
    GPU_RENDERPASS_TYPE_SECONDARY_COMMAND_BUFFERS
} GpuRenderPassType;

typedef enum
{
    GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER = BIT( 0 ),
    GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER = BIT( 1 )
} GpuRenderPassFlags;

class GpuRenderPass
{
  public:
    GpuRenderPass();
    ~GpuRenderPass();

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
