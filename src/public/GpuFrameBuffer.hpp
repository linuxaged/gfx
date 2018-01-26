#pragma once
#include "Gfx.hpp"
#include "GpuBuffer.hpp"
#include "GpuTexture.hpp"
namespace lxd
{
class GpuRenderPass;
class GpuWindow;

enum GpuMsaaMode
{
    MSAA_OFF,
    MSAA_RESOLVE,
    MSAA_BLIT
};

class GpuFramebuffer
{
  public:
    GpuFramebuffer(GpuContext* context);
    ~GpuFramebuffer();

    bool CreateFromSwapchain( GpuWindow* window, GpuRenderPass* renderPass );

	GpuContext& context;
    GpuTexture*    colorTextures;
    GpuTexture     renderTexture;
    GpuDepthBuffer depthBuffer;
    VkImageView*   textureViews;
    VkImageView*   renderViews;
    VkFramebuffer* framebuffers;
    GpuRenderPass* renderPass;
    GpuWindow*     window;
    int            swapchainCreateCount;
    int            width;
    int            height;
    int            numLayers;
    int            numBuffers;
    int            currentBuffer;
    int            currentLayer;
};

} // namespace lxd
