#include "GpuTexture.hpp"
#include "GpuDevice.hpp"
#include "GpuInstance.hpp"
#include "GpuWindow.hpp"
#include <algorithm>

#undef max

namespace lxd
{
GpuTexture::GpuTexture( GpuContext* context ) : context( *context ) {}

GpuTexture::~GpuTexture() {}

static int IntegerLog2( int i )
{
    int r = 0;
    int t;
    t = ( ( ~( ( i >> 16 ) + ~0U ) ) >> 27 ) & 0x10;
    r |= t;
    i >>= t;
    t = ( ( ~( ( i >> 8 ) + ~0U ) ) >> 28 ) & 0x08;
    r |= t;
    i >>= t;
    t = ( ( ~( ( i >> 4 ) + ~0U ) ) >> 29 ) & 0x04;
    r |= t;
    i >>= t;
    t = ( ( ~( ( i >> 2 ) + ~0U ) ) >> 30 ) & 0x02;
    r |= t;
    i >>= t;
    return ( r | ( i >> 1 ) );
}

VkImageLayout LayoutForTextureUsage( const GpuTextureUsage usage )
{
    return (
        ( usage == GPU_TEXTURE_USAGE_UNDEFINED )
            ? VK_IMAGE_LAYOUT_UNDEFINED
            : ( ( usage == GPU_TEXTURE_USAGE_GENERAL )
                    ? VK_IMAGE_LAYOUT_GENERAL
                    : ( ( usage == GPU_TEXTURE_USAGE_TRANSFER_SRC )
                            ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                            : ( ( usage == GPU_TEXTURE_USAGE_TRANSFER_DST )
                                    ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                    : ( ( usage == GPU_TEXTURE_USAGE_SAMPLED )
                                            ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                            : ( ( usage == GPU_TEXTURE_USAGE_STORAGE )
                                                    ? VK_IMAGE_LAYOUT_GENERAL
                                                    : ( ( usage ==
                                                          GPU_TEXTURE_USAGE_COLOR_ATTACHMENT )
                                                            ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                                            : ( ( usage ==
                                                                  GPU_TEXTURE_USAGE_PRESENTATION )
                                                                    ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                                                    : VK_IMAGE_LAYOUT_UNDEFINED ) ) ) ) ) ) ) );
}

VkAccessFlags AccessForTextureUsage( const GpuTextureUsage usage )
{
    return (
        ( usage == GPU_TEXTURE_USAGE_UNDEFINED )
            ? ( 0 )
            : ( ( usage == GPU_TEXTURE_USAGE_GENERAL )
                    ? ( 0 )
                    : ( ( usage == GPU_TEXTURE_USAGE_TRANSFER_SRC )
                            ? ( VK_ACCESS_TRANSFER_READ_BIT )
                            : ( ( usage == GPU_TEXTURE_USAGE_TRANSFER_DST )
                                    ? ( VK_ACCESS_TRANSFER_WRITE_BIT )
                                    : ( ( usage == GPU_TEXTURE_USAGE_SAMPLED )
                                            ? ( VK_ACCESS_SHADER_READ_BIT )
                                            : ( ( usage == GPU_TEXTURE_USAGE_STORAGE )
                                                    ? ( VK_ACCESS_SHADER_READ_BIT |
                                                        VK_ACCESS_SHADER_WRITE_BIT )
                                                    : ( ( usage ==
                                                          GPU_TEXTURE_USAGE_COLOR_ATTACHMENT )
                                                            ? ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT )
                                                            : ( ( usage ==
                                                                  GPU_TEXTURE_USAGE_PRESENTATION )
                                                                    ? ( VK_ACCESS_MEMORY_READ_BIT )
                                                                    : 0 ) ) ) ) ) ) ) );
}

VkPipelineStageFlags PipelineStagesForTextureUsage( const GpuTextureUsage usage, const bool from )
{
    return (
        ( usage == GPU_TEXTURE_USAGE_UNDEFINED )
            ? ( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT )
            : ( ( usage == GPU_TEXTURE_USAGE_GENERAL )
                    ? ( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT )
                    : ( ( usage == GPU_TEXTURE_USAGE_TRANSFER_SRC )
                            ? ( VK_PIPELINE_STAGE_TRANSFER_BIT )
                            : ( ( usage == GPU_TEXTURE_USAGE_TRANSFER_DST )
                                    ? ( VK_PIPELINE_STAGE_TRANSFER_BIT )
                                    : ( ( usage == GPU_TEXTURE_USAGE_SAMPLED )
                                            ? ( VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT )
                                            : ( ( usage == GPU_TEXTURE_USAGE_STORAGE )
                                                    ? ( VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT )
                                                    : ( ( usage ==
                                                          GPU_TEXTURE_USAGE_COLOR_ATTACHMENT )
                                                            ? ( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT )
                                                            : ( ( usage ==
                                                                  GPU_TEXTURE_USAGE_PRESENTATION )
                                                                    ? ( from
                                                                            ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                                            : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT )
                                                                    : 0 ) ) ) ) ) ) ) );
}

void GpuTexture::ChangeUsage( VkCommandBuffer cmdBuffer, const GpuTextureUsage usage )
{
    assert( ( this->usageFlags & usage ) != 0 );

    const VkImageLayout newImageLayout = LayoutForTextureUsage( usage );

    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext                           = NULL;
    imageMemoryBarrier.srcAccessMask                   = AccessForTextureUsage( this->usage );
    imageMemoryBarrier.dstAccessMask                   = AccessForTextureUsage( usage );
    imageMemoryBarrier.oldLayout                       = this->imageLayout;
    imageMemoryBarrier.newLayout                       = newImageLayout;
    imageMemoryBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image                           = this->image;
    imageMemoryBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
    imageMemoryBarrier.subresourceRange.levelCount     = this->mipCount;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount     = this->layerCount;

    const VkPipelineStageFlags src_stages = PipelineStagesForTextureUsage( this->usage, true );
    const VkPipelineStageFlags dst_stages = PipelineStagesForTextureUsage( usage, false );
    const VkDependencyFlags    flags      = 0;

    VC( context.device->vkCmdPipelineBarrier( cmdBuffer, src_stages, dst_stages, flags, 0, NULL, 0,
                                              NULL, 1, &imageMemoryBarrier ) );

    this->usage       = usage;
    this->imageLayout = newImageLayout;
}
void GpuTexture::UpdateSampler()
{
    if ( this->sampler != VK_NULL_HANDLE )
    {
        VC( context.device->vkDestroySampler( context.device->device, this->sampler,
                                              VK_ALLOCATOR ) );
    }

    const VkSamplerMipmapMode  mipmapMode = ( ( this->filter == GPU_TEXTURE_FILTER_NEAREST )
                                                 ? VK_SAMPLER_MIPMAP_MODE_NEAREST
                                                 : ( ( this->filter == GPU_TEXTURE_FILTER_LINEAR )
                                                         ? VK_SAMPLER_MIPMAP_MODE_NEAREST
                                                         : ( VK_SAMPLER_MIPMAP_MODE_LINEAR ) ) );
    const VkSamplerAddressMode addressMode =
        ( ( this->wrapMode == GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE )
              ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
              : ( ( this->wrapMode == GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER )
                      ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
                      : ( VK_SAMPLER_ADDRESS_MODE_REPEAT ) ) );

    VkSamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = NULL;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter =
        ( this->filter == GPU_TEXTURE_FILTER_NEAREST ) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter =
        ( this->filter == GPU_TEXTURE_FILTER_NEAREST ) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode              = mipmapMode;
    samplerCreateInfo.addressModeU            = addressMode;
    samplerCreateInfo.addressModeV            = addressMode;
    samplerCreateInfo.addressModeW            = addressMode;
    samplerCreateInfo.mipLodBias              = 0.0f;
    samplerCreateInfo.anisotropyEnable        = ( this->maxAnisotropy > 1.0f );
    samplerCreateInfo.maxAnisotropy           = this->maxAnisotropy;
    samplerCreateInfo.compareEnable           = VK_FALSE;
    samplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.minLod                  = 0.0f;
    samplerCreateInfo.maxLod                  = (float)this->mipCount;
    samplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    VK( context.device->vkCreateSampler( context.device->device, &samplerCreateInfo, VK_ALLOCATOR,
                                         &this->sampler ) );
}

bool GpuTexture::CreateInternal( const char* fileName, const VkFormat format,
                                 const GpuSampleCount sampleCount, const int width,
                                 const int height, const int depth, const int layerCount,
                                 const int faceCount, const int mipCount,
                                 const GpuTextureUsageFlags usageFlags, const void* data,
                                 const size_t dataSize, const bool mipSizeStored,
                                 const VkMemoryPropertyFlagBits memFlags )
{
    assert( depth >= 0 );
    assert( layerCount >= 0 );
    assert( faceCount == 1 || faceCount == 6 );

    if ( width < 1 || width > 32768 || height < 1 || height > 32768 || depth < 0 || depth > 32768 )
    {
        Error( "%s: Invalid texture size (%dx%dx%d)", fileName, width, height, depth );
        return false;
    }

    if ( faceCount != 1 && faceCount != 6 )
    {
        Error( "%s: Cube maps must have 6 faces (%d)", fileName, faceCount );
        return false;
    }

    if ( faceCount == 6 && width != height )
    {
        Error( "%s: Cube maps must be square (%dx%d)", fileName, width, height );
        return false;
    }

    if ( depth > 0 && layerCount > 0 )
    {
        Error( "%s: 3D array textures not supported", fileName );
        return false;
    }

    const int maxDimension =
        width > height ? ( width > depth ? width : depth ) : ( height > depth ? height : depth );
    const int maxMipLevels = ( 1 + IntegerLog2( maxDimension ) );

    if ( mipCount > maxMipLevels )
    {
        Error( "%s: Too many mip levels (%d > %d)", fileName, mipCount, maxMipLevels );
        return false;
    }

    VkFormatProperties props;
    VC( context.device->instance->vkGetPhysicalDeviceFormatProperties(
        context.device->physicalDevice, format, &props ) );

    // If this image is sampled.
    if ( ( usageFlags & GPU_TEXTURE_USAGE_SAMPLED ) != 0 )
    {
        if ( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) == 0 )
        {
            Error( "%s: Texture format %d cannot be sampled", fileName, format );
            return false;
        }
    }
    // If this image is rendered to.
    if ( ( usageFlags & GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) != 0 )
    {
        if ( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ) == 0 )
        {
            Error( "%s: Texture format %d cannot be rendered to", fileName, format );
            return false;
        }
    }
    // If this image is used for storage.
    if ( ( usageFlags & GPU_TEXTURE_USAGE_STORAGE ) != 0 )
    {
        if ( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT ) == 0 )
        {
            Error( "%s: Texture format %d cannot be used for storage", fileName, format );
            return false;
        }
    }

    const int numStorageLevels = ( mipCount >= 1 ) ? mipCount : maxMipLevels;
    const int arrayLayerCount  = faceCount * std::max( layerCount, 1 );

    this->width       = width;
    this->height      = height;
    this->depth       = depth;
    this->layerCount  = arrayLayerCount;
    this->mipCount    = numStorageLevels;
    this->sampleCount = sampleCount;
    this->usage       = GPU_TEXTURE_USAGE_UNDEFINED;
    this->usageFlags  = usageFlags;
    this->wrapMode    = GPU_TEXTURE_WRAP_MODE_REPEAT;
    this->filter =
        ( numStorageLevels > 1 ) ? GPU_TEXTURE_FILTER_BILINEAR : GPU_TEXTURE_FILTER_LINEAR;
    this->maxAnisotropy = 1.0f;
    this->format        = format;

    const VkImageUsageFlags usage =
        // Must be able to copy to the image for initialization.
        ( ( usageFlags & GPU_TEXTURE_USAGE_TRANSFER_DST ) != 0 || data != NULL
              ? VK_IMAGE_USAGE_TRANSFER_DST_BIT
              : 0 ) |
        // Must be able to blit from the image to create mip maps.
        ( ( usageFlags & GPU_TEXTURE_USAGE_TRANSFER_SRC ) != 0 || ( data != NULL && mipCount < 1 )
              ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT
              : 0 ) |
        // If this image is sampled.
        ( ( usageFlags & GPU_TEXTURE_USAGE_SAMPLED ) != 0 ? VK_IMAGE_USAGE_SAMPLED_BIT : 0 ) |
        // If this image is rendered to.
        ( ( usageFlags & GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) != 0
              ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
              : 0 ) |
        // If this image is used for storage.
        ( ( usageFlags & GPU_TEXTURE_USAGE_STORAGE ) != 0 ? VK_IMAGE_USAGE_STORAGE_BIT : 0 );

    // Create tiled image.
    VkImageCreateInfo imageCreateInfo;
    imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext         = NULL;
    imageCreateInfo.flags         = ( faceCount == 6 ) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    imageCreateInfo.imageType     = ( depth > 0 ) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    imageCreateInfo.format        = format;
    imageCreateInfo.extent.width  = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth  = std::max( depth, 1 );
    imageCreateInfo.mipLevels     = numStorageLevels;
    imageCreateInfo.arrayLayers   = arrayLayerCount;
    imageCreateInfo.samples       = (VkSampleCountFlagBits)sampleCount;
    imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage         = usage;
    imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices   = NULL;
    imageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    VK( context.device->vkCreateImage( context.device->device, &imageCreateInfo, VK_ALLOCATOR,
                                       &this->image ) );

    VkMemoryRequirements memoryRequirements;
    VC( context.device->vkGetImageMemoryRequirements( context.device->device, this->image,
                                                      &memoryRequirements ) );

    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext          = NULL;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex =
        context.device->GetMemoryTypeIndex( memoryRequirements.memoryTypeBits, memFlags );

    VK( context.device->vkAllocateMemory( context.device->device, &memoryAllocateInfo, VK_ALLOCATOR,
                                          &this->memory ) );
    VK( context.device->vkBindImageMemory( context.device->device, this->image, this->memory, 0 ) );

    if ( data == NULL )
    {
        context.CreateSetupCmdBuffer();

        // Set optimal image layout for shader read access.
        {
            VkImageMemoryBarrier imageMemoryBarrier;
            imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext               = NULL;
            imageMemoryBarrier.srcAccessMask       = 0;
            imageMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
            imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image               = this->image;
            imageMemoryBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
            imageMemoryBarrier.subresourceRange.levelCount     = numStorageLevels;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount     = arrayLayerCount;

            const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            const VkPipelineStageFlags dst_stages =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // fixme: optimise
            const VkDependencyFlags flags = 0;

            VC( context.device->vkCmdPipelineBarrier( context.setupCommandBuffer, src_stages,
                                                      dst_stages, flags, 0, NULL, 0, NULL, 1,
                                                      &imageMemoryBarrier ) );
        }
        context.FlushSetupCmdBuffer();
    }
    else // Copy source data through a staging buffer.
    {
        assert( sampleCount == GPU_SAMPLE_COUNT_1 );

        context.CreateSetupCmdBuffer();

        // Set optimal image layout for transfer destination.
        {
            VkImageMemoryBarrier imageMemoryBarrier;
            imageMemoryBarrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext                       = NULL;
            imageMemoryBarrier.srcAccessMask               = 0;
            imageMemoryBarrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image                       = this->image;
            imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
            imageMemoryBarrier.subresourceRange.levelCount     = numStorageLevels;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount     = arrayLayerCount;

            const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
            const VkDependencyFlags    flags      = 0;

            VC( context.device->vkCmdPipelineBarrier( context.setupCommandBuffer, src_stages,
                                                      dst_stages, flags, 0, NULL, 0, NULL, 1,
                                                      &imageMemoryBarrier ) );
        }

        const int numDataLevels = ( mipCount >= 1 ) ? mipCount : 1;
        bool      compressed    = false;

        // Using a staging buffer to initialize the tiled image.
        VkBufferCreateInfo stagingBufferCreateInfo;
        stagingBufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferCreateInfo.pNext                 = NULL;
        stagingBufferCreateInfo.flags                 = 0;
        stagingBufferCreateInfo.size                  = dataSize;
        stagingBufferCreateInfo.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        stagingBufferCreateInfo.queueFamilyIndexCount = 0;
        stagingBufferCreateInfo.pQueueFamilyIndices   = NULL;

        VkBuffer stagingBuffer;
        VK( context.device->vkCreateBuffer( context.device->device, &stagingBufferCreateInfo,
                                            VK_ALLOCATOR, &stagingBuffer ) );

        VkMemoryRequirements stagingMemoryRequirements;
        VC( context.device->vkGetBufferMemoryRequirements( context.device->device, stagingBuffer,
                                                           &stagingMemoryRequirements ) );

        VkMemoryAllocateInfo stagingMemoryAllocateInfo;
        stagingMemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        stagingMemoryAllocateInfo.pNext           = NULL;
        stagingMemoryAllocateInfo.allocationSize  = stagingMemoryRequirements.size;
        stagingMemoryAllocateInfo.memoryTypeIndex = context.device->GetMemoryTypeIndex(
            stagingMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

        VkDeviceMemory stagingMemory;
        VK( context.device->vkAllocateMemory( context.device->device, &stagingMemoryAllocateInfo,
                                              VK_ALLOCATOR, &stagingMemory ) );
        VK( context.device->vkBindBufferMemory( context.device->device, stagingBuffer,
                                                stagingMemory, 0 ) );

        //        uint8_t *mapped;
        //        VK(context.device->vkMapMemory(context.device->device, stagingMemory, 0,
        //                                        stagingMemoryRequirements.size, 0, (void **) &mapped));
        //        memcpy(mapped, data, dataSize);
        //        VC(context.device->vkUnmapMemory(context.device->device, stagingMemory));

        // Make sure the CPU writes to the buffer are flushed.
        {
            VkBufferMemoryBarrier bufferMemoryBarrier;
            bufferMemoryBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferMemoryBarrier.pNext               = NULL;
            bufferMemoryBarrier.srcAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
            bufferMemoryBarrier.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
            bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.buffer              = stagingBuffer;
            bufferMemoryBarrier.offset              = 0;
            bufferMemoryBarrier.size                = dataSize;

            const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_HOST_BIT;
            const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
            const VkDependencyFlags    flags      = 0;

            VC( context.device->vkCmdPipelineBarrier( context.setupCommandBuffer, src_stages,
                                                      dst_stages, flags, 0, NULL, 1,
                                                      &bufferMemoryBarrier, 0, NULL ) );
        }

        // to store the pixels, only used when mipSizeStored equals true
        uint8_t* pixelsData = (uint8_t*)malloc( dataSize );
        uint8_t* SrcData    = (uint8_t*)data;
        uint32_t pixelsSize = 0;

        VkBufferImageCopy* bufferImageCopy = (VkBufferImageCopy*)malloc(
            numDataLevels * arrayLayerCount * std::max( depth, 1 ) * sizeof( VkBufferImageCopy ) );
        uint32_t bufferImageCopyIndex = 0;
        uint32_t dataOffset           = 0;
        for ( int mipLevel = 0; mipLevel < numDataLevels; mipLevel++ )
        {
            const uint32_t mipWidth  = ( width >> mipLevel ) >= 1 ? ( width >> mipLevel ) : 1;
            const uint32_t mipHeight = ( height >> mipLevel ) >= 1 ? ( height >> mipLevel ) : 1;
            const uint32_t mipDepth  = ( depth >> mipLevel ) >= 1 ? ( depth >> mipLevel ) : 1;

            uint32_t totalMipSize  = 0;
            uint32_t storedMipSize = 0;
            if ( mipSizeStored )
            {
                assert( dataOffset + 4 <= dataSize );
                storedMipSize = *(uint32_t*)&( ( (uint8_t*)data )[dataOffset] );
                dataOffset += 4;
            }

            for ( int layerIndex = 0; layerIndex < arrayLayerCount; layerIndex++ )
            {
                for ( int depthIndex = 0; depthIndex < mipDepth; depthIndex++ )
                {
                    bufferImageCopy[bufferImageCopyIndex].bufferOffset      = 0;
                    bufferImageCopy[bufferImageCopyIndex].bufferRowLength   = 0;
                    bufferImageCopy[bufferImageCopyIndex].bufferImageHeight = 0;
                    bufferImageCopy[bufferImageCopyIndex].imageSubresource.aspectMask =
                        VK_IMAGE_ASPECT_COLOR_BIT;
                    bufferImageCopy[bufferImageCopyIndex].imageSubresource.mipLevel = mipLevel;
                    bufferImageCopy[bufferImageCopyIndex].imageSubresource.baseArrayLayer =
                        layerIndex;
                    bufferImageCopy[bufferImageCopyIndex].imageSubresource.layerCount = 1;
                    bufferImageCopy[bufferImageCopyIndex].imageOffset.x               = 0;
                    bufferImageCopy[bufferImageCopyIndex].imageOffset.y               = 0;
                    bufferImageCopy[bufferImageCopyIndex].imageOffset.z               = depthIndex;
                    bufferImageCopy[bufferImageCopyIndex].imageExtent.width           = mipWidth;
                    bufferImageCopy[bufferImageCopyIndex].imageExtent.height          = mipHeight;
                    bufferImageCopy[bufferImageCopyIndex].imageExtent.depth           = 1;
                    bufferImageCopyIndex++;

                    uint32_t mipSize = 0;
                    switch ( format )
                    {
                            //
                            // 8 bits per component
                            //
                        case VK_FORMAT_R8_UNORM:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned char );
                            break;
                        }
                        case VK_FORMAT_R8G8_UNORM:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned char );
                            break;
                        }
                        case VK_FORMAT_R8G8B8A8_UNORM:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned char );
                            break;
                        }

                        case VK_FORMAT_R8_SNORM:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( char );
                            break;
                        }
                        case VK_FORMAT_R8G8_SNORM:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( char );
                            break;
                        }
                        case VK_FORMAT_R8G8B8_SNORM:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( char );
                            break;
                        }

                        case VK_FORMAT_R8_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned char );
                            break;
                        }
                        case VK_FORMAT_R8G8_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned char );
                            break;
                        }
                        case VK_FORMAT_R8G8B8_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned char );
                            break;
                        }

                        case VK_FORMAT_R8_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( char );
                            break;
                        }
                        case VK_FORMAT_R8G8_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( char );
                            break;
                        }
                        case VK_FORMAT_R8G8B8_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( char );
                            break;
                        }

                        case VK_FORMAT_R8_SRGB:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned char );
                            break;
                        }
                        case VK_FORMAT_R8G8_SRGB:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned char );
                            break;
                        }
                        case VK_FORMAT_R8G8B8A8_SRGB:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned char );
                            break;
                        }

                        //
                        // 8 bits per component, bgra
                        //
                        case GPU_TEXTURE_FORMAT_B8G8R8A8_UNORM:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned char );
                            break;
                        }
                        case GPU_TEXTURE_FORMAT_B8G8R8A8_SNORM:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( char );
                            break;
                        }
                        case GPU_TEXTURE_FORMAT_B8G8R8A8_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned char );
                            break;
                        }
                        case GPU_TEXTURE_FORMAT_B8G8R8A8_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( char );
                            break;
                        }

                        //
                        // 16 bits per component
                        //
                        case VK_FORMAT_R16_UNORM:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned short );
                            break;
                        }
                        case VK_FORMAT_R16G16_UNORM:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned short );
                            break;
                        }
                        case VK_FORMAT_R16G16B16A16_UNORM:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned short );
                            break;
                        }

                        case VK_FORMAT_R16_SNORM:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( short );
                            break;
                        }
                        case VK_FORMAT_R16G16_SNORM:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( short );
                            break;
                        }
                        case VK_FORMAT_R16G16B16A16_SNORM:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( short );
                            break;
                        }

                        case VK_FORMAT_R16_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned short );
                            break;
                        }
                        case VK_FORMAT_R16G16_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned short );
                            break;
                        }
                        case VK_FORMAT_R16G16B16A16_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned short );
                            break;
                        }

                        case VK_FORMAT_R16_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( short );
                            break;
                        }
                        case VK_FORMAT_R16G16_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( short );
                            break;
                        }
                        case VK_FORMAT_R16G16B16A16_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( short );
                            break;
                        }

                        case VK_FORMAT_R16_SFLOAT:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned short );
                            break;
                        }
                        case VK_FORMAT_R16G16_SFLOAT:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned short );
                            break;
                        }
                        case VK_FORMAT_R16G16B16A16_SFLOAT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned short );
                            break;
                        }

                        //
                        // 32 bits per component
                        //
                        case VK_FORMAT_R32_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( unsigned int );
                            break;
                        }
                        case VK_FORMAT_R32G32_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( unsigned int );
                            break;
                        }
                        case VK_FORMAT_R32G32B32A32_UINT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( unsigned int );
                            break;
                        }

                        case VK_FORMAT_R32_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( int );
                            break;
                        }
                        case VK_FORMAT_R32G32_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( int );
                            break;
                        }
                        case VK_FORMAT_R32G32B32A32_SINT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( int );
                            break;
                        }

                        case VK_FORMAT_R32_SFLOAT:
                        {
                            mipSize = mipHeight * mipWidth * 1 * sizeof( float );
                            break;
                        }
                        case VK_FORMAT_R32G32_SFLOAT:
                        {
                            mipSize = mipHeight * mipWidth * 2 * sizeof( float );
                            break;
                        }
                        case VK_FORMAT_R32G32B32A32_SFLOAT:
                        {
                            mipSize = mipHeight * mipWidth * 4 * sizeof( float );
                            break;
                        }

                        //
                        // S3TC/DXT/BC
                        //
                        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_BC2_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_BC3_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }

                        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_BC2_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_BC3_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }

                        case VK_FORMAT_BC4_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_BC5_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }

                        case VK_FORMAT_BC4_SNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_BC5_SNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }

                        //
                        // ETC
                        //
                        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }

                        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }

                        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }

                        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 8;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }

                        //
                        // ASTC
                        //
                        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 4 ) / 5 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 4 ) / 5 ) * ( ( mipWidth + 4 ) / 5 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 4 ) / 5 ) * ( ( mipWidth + 5 ) / 6 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 5 ) / 6 ) * ( ( mipWidth + 5 ) / 6 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 4 ) / 5 ) * ( ( mipWidth + 7 ) / 8 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 5 ) / 6 ) * ( ( mipWidth + 7 ) / 8 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 7 ) / 8 ) * ( ( mipWidth + 7 ) / 8 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 4 ) / 5 ) * ( ( mipWidth + 9 ) / 10 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 5 ) / 6 ) * ( ( mipWidth + 9 ) / 10 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 7 ) / 8 ) * ( ( mipWidth + 9 ) / 10 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
                        {
                            mipSize = ( ( mipHeight + 9 ) / 10 ) * ( ( mipWidth + 9 ) / 10 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
                        {
                            mipSize = ( ( mipHeight + 9 ) / 10 ) * ( ( mipWidth + 11 ) / 12 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
                        {
                            mipSize = ( ( mipHeight + 11 ) / 12 ) * ( ( mipWidth + 11 ) / 12 ) * 16;
                            compressed = true;
                            break;
                        }

                        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 3 ) / 4 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 3 ) / 4 ) * ( ( mipWidth + 4 ) / 5 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 4 ) / 5 ) * ( ( mipWidth + 4 ) / 5 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 4 ) / 5 ) * ( ( mipWidth + 5 ) / 6 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 5 ) / 6 ) * ( ( mipWidth + 5 ) / 6 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 4 ) / 5 ) * ( ( mipWidth + 7 ) / 8 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 5 ) / 6 ) * ( ( mipWidth + 7 ) / 8 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 7 ) / 8 ) * ( ( mipWidth + 7 ) / 8 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 4 ) / 5 ) * ( ( mipWidth + 9 ) / 10 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 5 ) / 6 ) * ( ( mipWidth + 9 ) / 10 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
                        {
                            mipSize    = ( ( mipHeight + 7 ) / 8 ) * ( ( mipWidth + 9 ) / 10 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
                        {
                            mipSize = ( ( mipHeight + 9 ) / 10 ) * ( ( mipWidth + 9 ) / 10 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
                        {
                            mipSize = ( ( mipHeight + 9 ) / 10 ) * ( ( mipWidth + 11 ) / 12 ) * 16;
                            compressed = true;
                            break;
                        }
                        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
                        {
                            mipSize = ( ( mipHeight + 11 ) / 12 ) * ( ( mipWidth + 11 ) / 12 ) * 16;
                            compressed = true;
                            break;
                        }

                        default:
                        {
                            Error( "%s: Unsupported texture format %d", fileName, format );
                            return false;
                        }
                    }

                    assert( dataOffset + mipSize <= dataSize );

                    if ( mipSizeStored )
                    {
                        std::memcpy( pixelsData + pixelsSize + totalMipSize, SrcData + dataOffset,
                                     mipSize );
                    }

                    totalMipSize += mipSize;
                    dataOffset += mipSize;
                    if ( mipSizeStored && ( depth <= 0 && layerCount <= 0 ) )
                    {
                        assert( mipSize == storedMipSize );
                        dataOffset += 3 - ( ( mipSize + 3 ) % 4 );
                    }
                }
            }
            if ( mipSizeStored )
            {
                pixelsSize += totalMipSize;
                if ( ( depth > 0 ) || ( layerCount > 0 ) )
                {
                    assert( totalMipSize == storedMipSize );
                    dataOffset += 3 - ( ( totalMipSize + 3 ) % 4 );
                }
            }
        }

        if ( mipSizeStored )
        {
            assert( pixelsSize <= dataSize );
        }

        uint8_t* mapped;
        VK( context.device->vkMapMemory( context.device->device, stagingMemory, 0,
                                         stagingMemoryRequirements.size, 0, (void**)&mapped ) );
        if ( mipSizeStored )
        {
            // TODO : must remove the imageSize bytes of data
            std::memcpy( mapped, pixelsData, pixelsSize );
        }
        else
        {
            std::memcpy( mapped, data, dataSize );
        }

        VC( context.device->vkUnmapMemory( context.device->device, stagingMemory ) );
        // free alloced memory
        free( pixelsData );

        assert( dataOffset == dataSize );

        VC( context.device->vkCmdCopyBufferToImage(
            context.setupCommandBuffer, stagingBuffer, this->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            numDataLevels * arrayLayerCount * std::max( depth, 1 ), bufferImageCopy ) );

        if ( mipCount < 1 )
        {
            assert( !compressed );
            UNUSED_PARM( compressed );

            // Generate mip levels for the tiled image in place.
            for ( int mipLevel = 1; mipLevel <= numStorageLevels; mipLevel++ )
            {
                const int prevMipLevel = mipLevel - 1;

                // Make sure any copies to the image are flushed and set optimal image layout for transfer source.
                {
                    VkImageMemoryBarrier imageMemoryBarrier;
                    imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.pNext               = NULL;
                    imageMemoryBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
                    imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageMemoryBarrier.image               = this->image;
                    imageMemoryBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageMemoryBarrier.subresourceRange.baseMipLevel   = prevMipLevel;
                    imageMemoryBarrier.subresourceRange.levelCount     = 1;
                    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
                    imageMemoryBarrier.subresourceRange.layerCount     = arrayLayerCount;

                    const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    const VkDependencyFlags    flags      = 0;

                    VC( context.device->vkCmdPipelineBarrier(
                        context.setupCommandBuffer, src_stages, dst_stages, flags, 0, NULL, 0, NULL,
                        1, &imageMemoryBarrier ) );
                }

                // Blit from the previous mip level to the current mip level.
                if ( mipLevel < numStorageLevels )
                {
                    VkImageBlit imageBlit;
                    imageBlit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.srcSubresource.mipLevel       = prevMipLevel;
                    imageBlit.srcSubresource.baseArrayLayer = 0;
                    imageBlit.srcSubresource.layerCount     = arrayLayerCount;
                    imageBlit.srcOffsets[0].x               = 0;
                    imageBlit.srcOffsets[0].y               = 0;
                    imageBlit.srcOffsets[0].z               = 0;
                    imageBlit.srcOffsets[1].x =
                        ( width >> prevMipLevel ) >= 1 ? ( width >> prevMipLevel ) : 0;
                    imageBlit.srcOffsets[1].y =
                        ( height >> prevMipLevel ) >= 1 ? ( height >> prevMipLevel ) : 0;
                    imageBlit.srcOffsets[1].z =
                        ( depth >> prevMipLevel ) >= 1 ? ( depth >> prevMipLevel ) : 1;
                    imageBlit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.dstSubresource.mipLevel       = mipLevel;
                    imageBlit.dstSubresource.baseArrayLayer = 0;
                    imageBlit.dstSubresource.layerCount     = arrayLayerCount;
                    imageBlit.dstOffsets[0].x               = 0;
                    imageBlit.dstOffsets[0].y               = 0;
                    imageBlit.dstOffsets[0].z               = 0;
                    imageBlit.dstOffsets[1].x =
                        ( width >> mipLevel ) >= 1 ? ( width >> mipLevel ) : 0;
                    imageBlit.dstOffsets[1].y =
                        ( height >> mipLevel ) >= 1 ? ( height >> mipLevel ) : 0;
                    imageBlit.dstOffsets[1].z =
                        ( depth >> mipLevel ) >= 1 ? ( depth >> mipLevel ) : 1;

                    VC( context.device->vkCmdBlitImage(
                        context.setupCommandBuffer, this->image,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, this->image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR ) );
                }
            }
        }

        // Make sure any copies or blits to the image are flushed and set optimal image layout for shader read access.
        {
            VkImageMemoryBarrier imageMemoryBarrier;
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.srcAccessMask =
                ( mipCount >= 1 ) ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask =
                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            imageMemoryBarrier.oldLayout = ( mipCount >= 1 ) ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                                             : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image               = this->image;
            imageMemoryBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
            imageMemoryBarrier.subresourceRange.levelCount     = numStorageLevels;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount     = arrayLayerCount;

            const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
            const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            const VkDependencyFlags    flags      = 0;

            VC( context.device->vkCmdPipelineBarrier( context.setupCommandBuffer, src_stages,
                                                      dst_stages, flags, 0, NULL, 0, NULL, 1,
                                                      &imageMemoryBarrier ) );
        }

        context.FlushSetupCmdBuffer();

        VC( context.device->vkFreeMemory( context.device->device, stagingMemory, VK_ALLOCATOR ) );
        VC( context.device->vkDestroyBuffer( context.device->device, stagingBuffer,
                                             VK_ALLOCATOR ) );
        free( bufferImageCopy );
    }

    this->usage       = GPU_TEXTURE_USAGE_SAMPLED;
    this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const VkImageViewType viewType =
        ( ( depth > 0 )
              ? VK_IMAGE_VIEW_TYPE_3D
              : ( ( faceCount == 6 ) ? ( ( layerCount > 0 ) ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
                                                            : VK_IMAGE_VIEW_TYPE_CUBE )
                                     : ( ( layerCount > 0 ) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                                                            : VK_IMAGE_VIEW_TYPE_2D ) ) );

    VkImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext                           = NULL;
    imageViewCreateInfo.flags                           = 0;
    imageViewCreateInfo.image                           = this->image;
    imageViewCreateInfo.viewType                        = viewType;
    imageViewCreateInfo.format                          = format;
    imageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    imageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    imageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    imageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    imageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.levelCount     = numStorageLevels;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = arrayLayerCount;

    VK( context.device->vkCreateImageView( context.device->device, &imageViewCreateInfo,
                                           VK_ALLOCATOR, &this->view ) );

    UpdateSampler();

    return true;
}

bool GpuTexture::Create2D( const GpuTextureFormat format, const GpuSampleCount sampleCount,
                           const int width, const int height, const int mipCount,
                           const GpuTextureUsageFlags usageFlags, const void* data,
                           const size_t dataSize )
{
    const int depth      = 0;
    const int layerCount = 0;
    const int faceCount  = 1;
    return CreateInternal( "data", (VkFormat)format, sampleCount, width, height, depth, layerCount,
                           faceCount, mipCount, usageFlags, data, dataSize, false );
}

bool GpuTexture::Create2DArray( const GpuTextureFormat format, const GpuSampleCount sampleCount,
                                const int width, const int height, const int layerCount,
                                const int mipCount, const GpuTextureUsageFlags usageFlags,
                                const void* data, const size_t dataSize )
{
    const int depth     = 0;
    const int faceCount = 1;
    return CreateInternal( "data", (VkFormat)format, sampleCount, width, height, depth, layerCount,
                           faceCount, mipCount, usageFlags, data, dataSize, false );
}

bool GpuTexture::CreateFromSwapchain( const GpuWindow* window, int index )
{
    assert( index >= 0 && index < (int)window->swapchain.imageCount );

    this->width       = window->swapchain.width;
    this->height      = window->swapchain.height;
    this->depth       = 1;
    this->layerCount  = 1;
    this->mipCount    = 1;
    this->sampleCount = GPU_SAMPLE_COUNT_1;
    this->usage       = GPU_TEXTURE_USAGE_UNDEFINED;
    this->usageFlags  = GPU_TEXTURE_USAGE_STORAGE | GPU_TEXTURE_USAGE_COLOR_ATTACHMENT |
                       GPU_TEXTURE_USAGE_PRESENTATION;
    this->wrapMode      = GPU_TEXTURE_WRAP_MODE_REPEAT;
    this->filter        = GPU_TEXTURE_FILTER_LINEAR;
    this->maxAnisotropy = 1.0f;
    this->format        = window->swapchain.internalFormat;
    this->imageLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    this->image         = window->swapchain.images[index];
    this->memory        = VK_NULL_HANDLE;
    this->view          = window->swapchain.views[index];
    this->sampler       = VK_NULL_HANDLE;
    UpdateSampler();

    return true;
}

bool GpuTexture::CreateFromKTX( const char* fileName, const unsigned char* buffer,
                                const size_t bufferSize )
{

#pragma pack( 1 )
    typedef struct
    {
        unsigned char identifier[12];
        unsigned int  endianness;
        unsigned int  glType;
        unsigned int  glTypeSize;
        unsigned int  glFormat;
        unsigned int  glInternalFormat;
        unsigned int  glBaseInternalFormat;
        unsigned int  pixelWidth;
        unsigned int  pixelHeight;
        unsigned int  pixelDepth;
        unsigned int  numberOfArrayElements;
        unsigned int  numberOfFaces;
        unsigned int  numberOfMipmapLevels;
        unsigned int  bytesOfKeyValueData;
    } GlHeaderKTX_t;
#pragma pack()

    if ( bufferSize < sizeof( GlHeaderKTX_t ) )
    {
        Error( "%s: Invalid KTX file", fileName );
        return false;
    }

    const unsigned char fileIdentifier[12] = {
        (unsigned char)'\xAB', 'K',  'T',  'X',    ' ', '1', '1',
        (unsigned char)'\xBB', '\r', '\n', '\x1A', '\n'
    };

    const GlHeaderKTX_t* header = (GlHeaderKTX_t*)buffer;
    if ( memcmp( header->identifier, fileIdentifier, sizeof( fileIdentifier ) ) != 0 )
    {
        Error( "%s: Invalid KTX file", fileName );
        return false;
    }
    // only support little endian
    if ( header->endianness != 0x04030201 )
    {
        Error( "%s: KTX file has wrong endianess", fileName );
        return false;
    }
    // skip the key value data
    const size_t startTex = sizeof( GlHeaderKTX_t ) + header->bytesOfKeyValueData;
    if ( ( startTex < sizeof( GlHeaderKTX_t ) ) || ( startTex >= bufferSize ) )
    {
        Error( "%s: Invalid KTX header sizes", fileName );
        return false;
    }

    const unsigned int derivedFormat = glGetFormatFromInternalFormat( header->glInternalFormat );
    const unsigned int derivedType   = glGetTypeFromInternalFormat( header->glInternalFormat );

    UNUSED_PARM( derivedFormat );
    UNUSED_PARM( derivedType );

    // The glFormat and glType must either both be zero or both be non-zero.
    assert( ( header->glFormat == 0 ) == ( header->glType == 0 ) );
    // Uncompressed glTypeSize must be 1, 2, 4 or 8.
    assert( header->glFormat == 0 || header->glTypeSize == 1 || header->glTypeSize == 2 ||
            header->glTypeSize == 4 || header->glTypeSize == 8 );
    // Uncompressed glFormat must match the format derived from glInternalFormat.
    assert( header->glFormat == 0 || header->glFormat == derivedFormat );
    // Uncompressed glType must match the type derived from glInternalFormat.
    assert( header->glFormat == 0 || header->glType == derivedType );
    // Uncompressed glBaseInternalFormat must be the same as glFormat.
    assert( header->glFormat == 0 || header->glBaseInternalFormat == header->glFormat );
    // Compressed glTypeSize must be 1.
    assert( header->glFormat != 0 || header->glTypeSize == 1 );
    // Compressed glBaseInternalFormat must match the format drived from glInternalFormat.
    assert( header->glFormat != 0 || header->glBaseInternalFormat == derivedFormat );

    const int      numberOfFaces = ( header->numberOfFaces >= 1 ) ? header->numberOfFaces : 1;
    const VkFormat format        = vkGetFormatFromOpenGLInternalFormat( header->glInternalFormat );

    return CreateInternal( fileName, format, GPU_SAMPLE_COUNT_1, header->pixelWidth,
                           header->pixelHeight, header->pixelDepth, header->numberOfArrayElements,
                           numberOfFaces, header->numberOfMipmapLevels, GPU_TEXTURE_USAGE_SAMPLED,
                           buffer + startTex, bufferSize - startTex, true );
}
} // namespace lxd
