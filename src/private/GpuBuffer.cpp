#include "GpuBuffer.hpp"
#include "GpuDevice.hpp"
#include "GpuContext.hpp"
namespace lxd
{

VkBufferUsageFlags GpuBuffer::GetBufferUsage( const GpuBufferType type )
{
    return ( ( type == GPU_BUFFER_TYPE_VERTEX )
                 ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                 : ( ( type == GPU_BUFFER_TYPE_INDEX )
                         ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                         : ( ( type == GPU_BUFFER_TYPE_UNIFORM )
                                 ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                                 : ( ( type == GPU_BUFFER_TYPE_STORAGE )
                                         ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                         : 0 ) ) ) );
}

VkAccessFlags GpuBuffer::GetBufferAccess( const GpuBufferType type )
{
    return (
        ( type == GPU_BUFFER_TYPE_INDEX )
            ? VK_ACCESS_INDEX_READ_BIT
            : ( ( type == GPU_BUFFER_TYPE_VERTEX )
                    ? VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
                    : ( ( type == GPU_BUFFER_TYPE_UNIFORM )
                            ? VK_ACCESS_UNIFORM_READ_BIT
                            : ( ( type == GPU_BUFFER_TYPE_STORAGE )
                                    ? ( VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT )
                                    : 0 ) ) ) );
}
GpuBuffer::GpuBuffer( GpuContext* context, const GpuBufferType type, const size_t dataSize,
                      const void* data, const bool hostVisible, const bool dynamic )
    : context( *context )
{
    assert( dataSize <= context->device->physicalDeviceProperties.limits.maxStorageBufferRange );

    this->type  = type;
    this->size  = dataSize;
    this->owner = true;

    VkBufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size  = dataSize;
    bufferCreateInfo.usage =
        GpuBuffer::GetBufferUsage( type ) |
        ( hostVisible ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT );
    bufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices   = nullptr;

    VK( context->device->vkCreateBuffer( context->device->device, &bufferCreateInfo, VK_ALLOCATOR,
                                         &this->buffer ) );

    this->flags =
        hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkMemoryRequirements memoryRequirements;
    VC( context->device->vkGetBufferMemoryRequirements( context->device->device, this->buffer,
                                                        &memoryRequirements ) );

    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext          = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex =
        context->device->GetMemoryTypeIndex( memoryRequirements.memoryTypeBits, this->flags );

    VK( context->device->vkAllocateMemory( context->device->device, &memoryAllocateInfo,
                                           VK_ALLOCATOR, &this->memory ) );
    VK( context->device->vkBindBufferMemory( context->device->device, this->buffer, this->memory,
                                             0 ) );

    if ( data != nullptr )
    {
        if ( hostVisible )
        {
            void* mapped;
            VK( context->device->vkMapMemory( context->device->device, this->memory, 0,
                                              memoryRequirements.size, 0, &mapped ) );
            memcpy( mapped, data, dataSize );
            VC( context->device->vkUnmapMemory( context->device->device, this->memory ) );

            VkMappedMemoryRange mappedMemoryRange;
            mappedMemoryRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemoryRange.pNext  = nullptr;
            mappedMemoryRange.memory = this->memory;
            mappedMemoryRange.offset = 0;
            mappedMemoryRange.size   = VK_WHOLE_SIZE;
            VC( context->device->vkFlushMappedMemoryRanges( context->device->device, 1,
                                                            &mappedMemoryRange ) );
        }
        else
        {
            VkBufferCreateInfo stagingBufferCreateInfo;
            stagingBufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            stagingBufferCreateInfo.pNext                 = nullptr;
            stagingBufferCreateInfo.flags                 = 0;
            stagingBufferCreateInfo.size                  = dataSize;
            stagingBufferCreateInfo.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stagingBufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
            stagingBufferCreateInfo.queueFamilyIndexCount = 0;
            stagingBufferCreateInfo.pQueueFamilyIndices   = nullptr;

            VkBuffer srcBuffer;
            VK( context->device->vkCreateBuffer( context->device->device, &stagingBufferCreateInfo,
                                                 VK_ALLOCATOR, &srcBuffer ) );

            VkMemoryRequirements stagingMemoryRequirements;
            VC( context->device->vkGetBufferMemoryRequirements( context->device->device, srcBuffer,
                                                                &stagingMemoryRequirements ) );

            VkMemoryAllocateInfo stagingMemoryAllocateInfo;
            stagingMemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            stagingMemoryAllocateInfo.pNext           = nullptr;
            stagingMemoryAllocateInfo.allocationSize  = stagingMemoryRequirements.size;
            stagingMemoryAllocateInfo.memoryTypeIndex = context->device->GetMemoryTypeIndex(
                stagingMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

            VkDeviceMemory srcMemory;
            VK( context->device->vkAllocateMemory(
                context->device->device, &stagingMemoryAllocateInfo, VK_ALLOCATOR, &srcMemory ) );
            VK( context->device->vkBindBufferMemory( context->device->device, srcBuffer, srcMemory,
                                                     0 ) );

            void* mapped;
            VK( context->device->vkMapMemory( context->device->device, srcMemory, 0,
                                              stagingMemoryRequirements.size, 0, &mapped ) );
            memcpy( mapped, data, dataSize );
            VC( context->device->vkUnmapMemory( context->device->device, srcMemory ) );

            VkMappedMemoryRange mappedMemoryRange;
            mappedMemoryRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemoryRange.pNext  = nullptr;
            mappedMemoryRange.memory = srcMemory;
            mappedMemoryRange.offset = 0;
            mappedMemoryRange.size   = VK_WHOLE_SIZE;
            VC( context->device->vkFlushMappedMemoryRanges( context->device->device, 1,
                                                            &mappedMemoryRange ) );

            context->CreateSetupCmdBuffer();

            VkBufferCopy bufferCopy;
            bufferCopy.srcOffset = 0;
            bufferCopy.dstOffset = 0;
            bufferCopy.size      = dataSize;

            VC( context->device->vkCmdCopyBuffer( context->setupCommandBuffer, srcBuffer,
                                                  this->buffer, 1, &bufferCopy ) );

            context->FlushSetupCmdBuffer();

            VC( context->device->vkDestroyBuffer( context->device->device, srcBuffer,
                                                  VK_ALLOCATOR ) );
            VC( context->device->vkFreeMemory( context->device->device, srcMemory, VK_ALLOCATOR ) );
        }
    }
}

GpuBuffer::~GpuBuffer()
{
    if ( this->mapped != nullptr )
    {
        VC( context.device->vkUnmapMemory( context.device->device, this->memory ) );
    }
    if ( this->owner )
    {
        VC( context.device->vkDestroyBuffer( context.device->device, this->buffer, VK_ALLOCATOR ) );
        VC( context.device->vkFreeMemory( context.device->device, this->memory, VK_ALLOCATOR ) );
    }
}
//
//
//
VkFormat GpuDepthBuffer::InternalSurfaceDepthFormat( const GpuSurfaceDepthFormat depthFormat )
{
    return ( ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_D16 )
                 ? VK_FORMAT_D16_UNORM
                 : ( ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_D24 ) ? VK_FORMAT_D24_UNORM_S8_UINT
                                                                     : VK_FORMAT_UNDEFINED ) );
}

GpuDepthBuffer::GpuDepthBuffer( GpuContext* context, const GpuSurfaceDepthFormat depthFormat,
                                const GpuSampleCount sampleCount, const int width, const int height,
                                const int numLayers )
    : context( *context )
{
    assert( width >= 1 );
    assert( height >= 1 );
    assert( numLayers >= 1 );

    this->format = depthFormat;

    if ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_NONE )
    {
        this->internalFormat = VK_FORMAT_UNDEFINED;
        return;
    }

    this->internalFormat = GpuDepthBuffer::InternalSurfaceDepthFormat( depthFormat );

    VkImageCreateInfo imageCreateInfo;
    imageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext                 = nullptr;
    imageCreateInfo.flags                 = 0;
    imageCreateInfo.imageType             = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format                = this->internalFormat;
    imageCreateInfo.extent.width          = width;
    imageCreateInfo.extent.height         = height;
    imageCreateInfo.extent.depth          = 1;
    imageCreateInfo.mipLevels             = 1;
    imageCreateInfo.arrayLayers           = numLayers;
    imageCreateInfo.samples               = static_cast<VkSampleCountFlagBits>(sampleCount);
    imageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices   = nullptr;
    imageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    VK( context->device->vkCreateImage( context->device->device, &imageCreateInfo, VK_ALLOCATOR,
                                        &this->image ) );

    VkMemoryRequirements memoryRequirements;
    VC( context->device->vkGetImageMemoryRequirements( context->device->device, this->image,
                                                       &memoryRequirements ) );

    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext           = nullptr;
    memoryAllocateInfo.allocationSize  = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = context->device->GetMemoryTypeIndex(
        memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    VK( context->device->vkAllocateMemory( context->device->device, &memoryAllocateInfo,
                                           VK_ALLOCATOR, &this->memory ) );
    VK( context->device->vkBindImageMemory( context->device->device, this->image, this->memory,
                                            0 ) );

    this->views    = static_cast<VkImageView*>( malloc( numLayers * sizeof( VkImageView ) ) );
    this->numViews = numLayers;

    for ( int layerIndex = 0; layerIndex < numLayers; layerIndex++ )
    {
        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext        = nullptr;
        imageViewCreateInfo.flags        = 0;
        imageViewCreateInfo.image        = this->image;
        imageViewCreateInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format       = this->internalFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreateInfo.subresourceRange.levelCount     = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = layerIndex;
        imageViewCreateInfo.subresourceRange.layerCount     = 1;

        VK( context->device->vkCreateImageView( context->device->device, &imageViewCreateInfo,
                                                VK_ALLOCATOR, &this->views[layerIndex] ) );
    }

    //
    // Set optimal image layout
    //

    {
        context->CreateSetupCmdBuffer();

        VkImageMemoryBarrier imageMemoryBarrier;
        imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext               = nullptr;
        imageMemoryBarrier.srcAccessMask       = 0;
        imageMemoryBarrier.dstAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image               = this->image;
        imageMemoryBarrier.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
        imageMemoryBarrier.subresourceRange.levelCount     = 1;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount     = numLayers;

        const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        const VkDependencyFlags    flags      = 0;

        VC( context->device->vkCmdPipelineBarrier( context->setupCommandBuffer, src_stages,
                                                   dst_stages, flags, 0, nullptr, 0, nullptr, 1,
                                                   &imageMemoryBarrier ) );

        context->FlushSetupCmdBuffer();
    }

    this->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

GpuDepthBuffer::~GpuDepthBuffer()
{

    if ( this->internalFormat == VK_FORMAT_UNDEFINED )
    {
        return;
    }

    for ( int viewIndex = 0; viewIndex < this->numViews; viewIndex++ )
    {
        VC( context.device->vkDestroyImageView( context.device->device, this->views[viewIndex],
                                                VK_ALLOCATOR ) );
    }
    VC( context.device->vkDestroyImage( context.device->device, this->image, VK_ALLOCATOR ) );
    VC( context.device->vkFreeMemory( context.device->device, this->memory, VK_ALLOCATOR ) );

    free( this->views );
}

} // namespace lxd
