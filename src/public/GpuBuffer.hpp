#pragma once

#include "GpuContext.hpp"

namespace lxd
{

enum GpuBufferType
{
    GPU_BUFFER_TYPE_VERTEX,
    GPU_BUFFER_TYPE_INDEX,
    GPU_BUFFER_TYPE_UNIFORM,
    GPU_BUFFER_TYPE_STORAGE
};

class GpuBuffer
{
  public:
    GpuBuffer( GpuContext* context, const GpuBufferType type, const size_t dataSize,
               const void* data, const bool hostVisible, const bool dynamic = false );
    ~GpuBuffer();

    static VkBufferUsageFlags GetBufferUsage( const GpuBufferType type );
    static VkAccessFlags      GetBufferAccess( const GpuBufferType type );

  public:
    GpuContext&           context;
    GpuBuffer*            next        = nullptr;
    int                   unusedCount = 0;
    GpuBufferType         type        = {};
    size_t                size        = 0;
    VkMemoryPropertyFlags flags       = {};
    VkBuffer              buffer      = nullptr;
    VkDeviceMemory        memory      = nullptr;
    void*                 mapped      = nullptr;
    bool                  owner       = false;
};

class GpuDepthBuffer
{
  public:
    GpuDepthBuffer( GpuContext* context ) : context( *context ) {}
    GpuDepthBuffer( GpuContext* context, const GpuSurfaceDepthFormat depthFormat,
                    const GpuSampleCount sampleCount, const int width, const int height,
                    const int numLayers );
    ~GpuDepthBuffer();

    static VkFormat InternalSurfaceDepthFormat( const GpuSurfaceDepthFormat depthFormat );

  public:
    GpuContext&           context;
    GpuSurfaceDepthFormat format         = {};
    VkFormat              internalFormat = {};
    VkImageLayout         imageLayout    = {};
    VkImage               image          = nullptr;
    VkDeviceMemory        memory         = nullptr;
    VkImageView*          views          = nullptr;
    int                   numViews       = 0;
};
} // namespace lxd
