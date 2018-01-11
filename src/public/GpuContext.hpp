#pragma once

#include "Gfx.hpp"

namespace lxd
{

class GpuDevice;

enum GpuSurfaceColorFormat
{
    GPU_SURFACE_COLOR_FORMAT_R5G6B5,
    GPU_SURFACE_COLOR_FORMAT_B5G6R5,
    GPU_SURFACE_COLOR_FORMAT_R8G8B8A8,
    GPU_SURFACE_COLOR_FORMAT_B8G8R8A8,
    GPU_SURFACE_COLOR_FORMAT_MAX
};

enum GpuSurfaceDepthFormat
{
    GPU_SURFACE_DEPTH_FORMAT_NONE,
    GPU_SURFACE_DEPTH_FORMAT_D16,
    GPU_SURFACE_DEPTH_FORMAT_D24,
    GPU_SURFACE_DEPTH_FORMAT_MAX
};

enum GpuSampleCount
{
    GPU_SAMPLE_COUNT_1  = VK_SAMPLE_COUNT_1_BIT,
    GPU_SAMPLE_COUNT_2  = VK_SAMPLE_COUNT_2_BIT,
    GPU_SAMPLE_COUNT_4  = VK_SAMPLE_COUNT_4_BIT,
    GPU_SAMPLE_COUNT_8  = VK_SAMPLE_COUNT_8_BIT,
    GPU_SAMPLE_COUNT_16 = VK_SAMPLE_COUNT_16_BIT,
    GPU_SAMPLE_COUNT_32 = VK_SAMPLE_COUNT_32_BIT,
    GPU_SAMPLE_COUNT_64 = VK_SAMPLE_COUNT_64_BIT,
};

struct GpuLimits
{
    size_t maxPushConstantsSize;
    int    maxSamples;
};

class GpuContext
{
  public:
    struct GpuSurfaceBits
    {
        unsigned char redBits;
        unsigned char greenBits;
        unsigned char blueBits;
        unsigned char alphaBits;
        unsigned char colorBits;
        unsigned char depthBits;
    };

    GpuContext( GpuDevice* device, const int queueIndex );
    ~GpuContext();

    GpuSurfaceBits BitsForSurfaceFormat( const GpuSurfaceColorFormat colorFormat,
                                         const GpuSurfaceDepthFormat depthFormat );
    GLenum         InternalSurfaceColorFormat( const GpuSurfaceColorFormat colorFormat );
    GLenum         InternalSurfaceDepthFormat( const GpuSurfaceDepthFormat depthFormat );
    void           CreateShared( const GpuContext* other, const int queueIndex );
    void           WaitIdle();
    void           SetCurrent();
    void           UnsetCurrent();
    bool           CheckCurrent();
    void           GetLimits( GpuLimits* limits );

    void CreateSetupCmdBuffer();
    void FlushSetupCmdBuffer();

  public:
    GpuDevice*      device;
    uint32_t        queueFamilyIndex;
    uint32_t        queueIndex;
    VkQueue         queue;
    VkCommandPool   commandPool;
    VkPipelineCache pipelineCache;
    VkCommandBuffer setupCommandBuffer;
};

} // namespace lxd
