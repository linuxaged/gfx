#include "GpuRenderPass.hpp"
#include "GpuSwapchain.hpp"
#include "GpuBuffer.hpp"

namespace lxd
{
GpuRenderPass::GpuRenderPass()
{
    assert( ( context->device->physicalDeviceProperties.limits.framebufferColorSampleCounts &
              (VkSampleCountFlags)sampleCount ) != 0 );
    assert( ( context->device->physicalDeviceProperties.limits.framebufferDepthSampleCounts &
              (VkSampleCountFlags)sampleCount ) != 0 );

    this->type                = type;
    this->flags               = flags;
    this->colorFormat         = colorFormat;
    this->depthFormat         = depthFormat;
    this->sampleCount         = sampleCount;
    this->internalColorFormat = GpuSwapchain::InternalSurfaceColorFormat( colorFormat );
    this->internalDepthFormat = GpuDepthBuffer::InternalSurfaceDepthFormat( depthFormat );

    uint32_t                attachmentCount = 0;
    VkAttachmentDescription attachments[3];

    // Optionally use a multi-sampled attachment.
    if ( sampleCount > GPU_SAMPLE_COUNT_1 )
    {
        attachments[attachmentCount].flags   = 0;
        attachments[attachmentCount].format  = this->internalColorFormat;
        attachments[attachmentCount].samples = (VkSampleCountFlagBits)sampleCount;
        attachments[attachmentCount].loadOp =
            ( ( flags & GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER ) != 0 )
                ? VK_ATTACHMENT_LOAD_OP_CLEAR
                : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachmentCount].storeOp = ( EXPLICIT_RESOLVE != 0 )
                                                   ? VK_ATTACHMENT_STORE_OP_STORE
                                                   : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[attachmentCount].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[attachmentCount].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[attachmentCount].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentCount++;
    }
    // Either render directly to, or resolve to the single-sample attachment.
    if ( sampleCount <= GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0 )
    {
        attachments[attachmentCount].flags   = 0;
        attachments[attachmentCount].format  = this->internalColorFormat;
        attachments[attachmentCount].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[attachmentCount].loadOp =
            ( ( flags & GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER ) != 0 &&
              sampleCount <= GPU_SAMPLE_COUNT_1 )
                ? VK_ATTACHMENT_LOAD_OP_CLEAR
                : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachmentCount].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[attachmentCount].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[attachmentCount].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[attachmentCount].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentCount++;
    }
    // Optionally use a depth buffer.
    if ( this->internalDepthFormat != VK_FORMAT_UNDEFINED )
    {
        attachments[attachmentCount].flags   = 0;
        attachments[attachmentCount].format  = this->internalDepthFormat;
        attachments[attachmentCount].samples = (VkSampleCountFlagBits)sampleCount;
        attachments[attachmentCount].loadOp =
            ( ( flags & GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER ) != 0 )
                ? VK_ATTACHMENT_LOAD_OP_CLEAR
                : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachmentCount].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[attachmentCount].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[attachmentCount].initialLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachmentCount++;
    }

    VkAttachmentReference colorAttachmentReference;
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveAttachmentReference;
    resolveAttachmentReference.attachment = 1;
    resolveAttachmentReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment =
        ( sampleCount > GPU_SAMPLE_COUNT_1 && EXPLICIT_RESOLVE == 0 ) ? 2 : 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    subpassDescription.flags                = 0;
    subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments    = NULL;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments    = &colorAttachmentReference;
    subpassDescription.pResolveAttachments =
        ( sampleCount > GPU_SAMPLE_COUNT_1 && EXPLICIT_RESOLVE == 0 ) ? &resolveAttachmentReference
                                                                      : NULL;
    subpassDescription.pDepthStencilAttachment =
        ( this->internalDepthFormat != VK_FORMAT_UNDEFINED ) ? &depthAttachmentReference : NULL;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = NULL;

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext           = NULL;
    renderPassCreateInfo.flags           = 0;
    renderPassCreateInfo.attachmentCount = attachmentCount;
    renderPassCreateInfo.pAttachments    = attachments;
    renderPassCreateInfo.subpassCount    = 1;
    renderPassCreateInfo.pSubpasses      = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies   = NULL;

    VK( context->device->vkCreateRenderPass( context->device->device, &renderPassCreateInfo,
                                             VK_ALLOCATOR, &this->renderPass ) );
}

GpuRenderPass::~GpuRenderPass()
{
    VC( context->device->vkDestroyRenderPass( context->device->device, this->renderPass,
                                              VK_ALLOCATOR ) );
}
} // namespace lxd
