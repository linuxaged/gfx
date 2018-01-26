#include "GpuFrameBuffer.hpp"
#include "GpuWindow.hpp"
#include "GpuRenderPass.hpp"

namespace lxd
{
GpuFramebuffer::GpuFramebuffer( GpuContext* context )
    : context( *context ), renderTexture( context ), depthBuffer( context )
{
}

GpuFramebuffer::~GpuFramebuffer() {}

bool GpuFramebuffer::CreateFromSwapchain( GpuWindow* window, GpuRenderPass* renderPass )
{
    assert( window->windowWidth >= 1 &&
            window->windowWidth <=
                (int)window->context.device->physicalDeviceProperties.limits.maxFramebufferWidth );
    assert( window->windowHeight >= 1 &&
            window->windowHeight <=
                (int)window->context.device->physicalDeviceProperties.limits.maxFramebufferHeight );
    assert( window->sampleCount == renderPass->sampleCount );

    this->renderPass           = renderPass;
    this->window               = window;
    this->swapchainCreateCount = window->swapchainCreateCount;
    this->width                = window->windowWidth;
    this->height               = window->windowHeight;
    this->numLayers            = 1;
    this->numBuffers           = 3; // Default to 3 for late swapchain creation on Android.
    this->currentBuffer        = 0;
    this->currentLayer         = 0;

    if ( window->swapchain.swapchain == VK_NULL_HANDLE )
    {
        return false;
    }

    assert( renderPass->internalColorFormat == window->swapchain.internalFormat );
    assert( renderPass->internalDepthFormat == window->depthBuffer.internalFormat );
    assert( this->numBuffers >= (int)window->swapchain.imageCount );

    this->colorTextures =
        (GpuTexture*)malloc( window->swapchain.imageCount * sizeof( GpuTexture ) );
    this->textureViews = NULL;
    this->renderViews  = NULL;
    this->framebuffers =
        (VkFramebuffer*)malloc( window->swapchain.imageCount * sizeof( VkFramebuffer ) );
    this->numBuffers = window->swapchain.imageCount;

    if ( renderPass->sampleCount > GPU_SAMPLE_COUNT_1 )
    {
        this->renderTexture.Create2D( (GpuTextureFormat)renderPass->internalColorFormat,
                                      renderPass->sampleCount, window->windowWidth,
                                      window->windowHeight, 1, GPU_TEXTURE_USAGE_COLOR_ATTACHMENT,
                                      NULL, 0 );

        window->context.CreateSetupCmdBuffer();
        this->renderTexture.ChangeUsage( window->context.setupCommandBuffer,
                                         GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );
        window->context.FlushSetupCmdBuffer();
    }

    for ( uint32_t imageIndex = 0; imageIndex < window->swapchain.imageCount; imageIndex++ )
    {
        assert( renderPass->colorFormat == window->colorFormat );
        assert( renderPass->depthFormat == window->depthFormat );
        this->colorTextures[imageIndex].CreateFromSwapchain( window, imageIndex );

        assert( window->windowWidth == this->colorTextures[imageIndex].width );
        assert( window->windowHeight == this->colorTextures[imageIndex].height );

        uint32_t    attachmentCount = 0;
        VkImageView attachments[3];

        if ( renderPass->sampleCount > GPU_SAMPLE_COUNT_1 )
        {
            attachments[attachmentCount++] = this->renderTexture.view;
        }
        if ( renderPass->sampleCount <= GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0 )
        {
            attachments[attachmentCount++] = this->colorTextures[imageIndex].view;
        }
        if ( renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
        {
            attachments[attachmentCount++] = window->depthBuffer.views[0];
        }

        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext           = NULL;
        framebufferCreateInfo.flags           = 0;
        framebufferCreateInfo.renderPass      = renderPass->renderPass;
        framebufferCreateInfo.attachmentCount = attachmentCount;
        framebufferCreateInfo.pAttachments    = attachments;
        framebufferCreateInfo.width           = window->windowWidth;
        framebufferCreateInfo.height          = window->windowHeight;
        framebufferCreateInfo.layers          = 1;

        VK( window->context.device->vkCreateFramebuffer( window->context.device->device,
                                                         &framebufferCreateInfo, VK_ALLOCATOR,
                                                         &this->framebuffers[imageIndex] ) );
    }

    return true;
}

} // namespace lxd
