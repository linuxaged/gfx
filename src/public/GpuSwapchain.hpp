#pragma once

#include "Gfx.hpp"
#include "GpuContext.hpp"

namespace lxd
{

struct GpuSwapchainBuffer
{
    uint32_t    imageIndex;
    VkSemaphore presentCompleteSemaphore;
    VkSemaphore renderingCompleteSemaphore;
};

class GpuSwapchain
{
  public:
    GpuSwapchain( GpuContext* context, const VkSurfaceKHR surface,
                  const GpuSurfaceColorFormat colorFormat, const int width, const int height,
                  const int swapInterval );
    ~GpuSwapchain();

    static VkFormat InternalSurfaceColorFormat( const GpuSurfaceColorFormat colorFormat );

  public:
    GpuContext&           context;
    GpuSurfaceColorFormat format         = {};
    VkFormat              internalFormat = {};
    VkColorSpaceKHR       colorSpace     = {};
    int                   width          = 0;
    int                   height         = 0;
    VkQueue               presentQueue   = nullptr;
    VkSwapchainKHR        swapchain      = nullptr;
    uint32_t              imageCount     = 0;
    VkImage*              images         = nullptr;
    VkImageView*          views          = nullptr;
    uint32_t              bufferCount    = 0;
    uint32_t              currentBuffer  = 0;
    GpuSwapchainBuffer*   buffers        = nullptr;
};
} // namespace lxd
