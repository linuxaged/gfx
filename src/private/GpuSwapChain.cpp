#include "GpuSwapChain.hpp"
#include "GpuDevice.hpp"
#include "GpuInstance.hpp"
#include <algorithm>
namespace lxd
{

VkFormat GpuSwapchain::InternalSurfaceColorFormat( const GpuSurfaceColorFormat colorFormat )
{
    return ( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 )
                 ? VK_FORMAT_R8G8B8A8_UNORM
                 : ( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 )
                         ? VK_FORMAT_B8G8R8A8_UNORM
                         : ( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R5G6B5 )
                                 ? VK_FORMAT_R5G6B5_UNORM_PACK16
                                 : ( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B5G6R5 )
                                         ? VK_FORMAT_B5G6R5_UNORM_PACK16
                                         : ( ( VK_FORMAT_UNDEFINED ) ) ) ) ) );
}

GpuSwapchain::GpuSwapchain( GpuContext* context, const VkSurfaceKHR surface,
                            const GpuSurfaceColorFormat colorFormat, const int width,
                            const int height, const int swapInterval )
    : context( *context )
{
    GpuDevice* device = context->device;

    if ( !device->foundSwapchainExtension )
    {
        Error( "foundSwapchainExtension == false!" );
    }

    // Get the list of formats that are supported.
    uint32_t formatCount;
    VK( device->instance->vkGetPhysicalDeviceSurfaceFormatsKHR( device->physicalDevice, surface,
                                                                &formatCount, nullptr ) );

    VkSurfaceFormatKHR* surfaceFormats =
        (VkSurfaceFormatKHR*)malloc( formatCount * sizeof( VkSurfaceFormatKHR ) );
    VK( device->instance->vkGetPhysicalDeviceSurfaceFormatsKHR( device->physicalDevice, surface,
                                                                &formatCount, surfaceFormats ) );

    const GpuSurfaceColorFormat
        desiredFormatTable[GPU_SURFACE_COLOR_FORMAT_MAX][GPU_SURFACE_COLOR_FORMAT_MAX] = {
            { GPU_SURFACE_COLOR_FORMAT_R5G6B5, GPU_SURFACE_COLOR_FORMAT_B5G6R5,
              GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 },
            { GPU_SURFACE_COLOR_FORMAT_B5G6R5, GPU_SURFACE_COLOR_FORMAT_R5G6B5,
              GPU_SURFACE_COLOR_FORMAT_B8G8R8A8, GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 },
            { GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_COLOR_FORMAT_B8G8R8A8,
              GPU_SURFACE_COLOR_FORMAT_R5G6B5, GPU_SURFACE_COLOR_FORMAT_B5G6R5 },
            { GPU_SURFACE_COLOR_FORMAT_B8G8R8A8, GPU_SURFACE_COLOR_FORMAT_R8G8B8A8,
              GPU_SURFACE_COLOR_FORMAT_B5G6R5, GPU_SURFACE_COLOR_FORMAT_R5G6B5 }
        };
    assert( GPU_SURFACE_COLOR_FORMAT_R5G6B5 == 0 );
    assert( GPU_SURFACE_COLOR_FORMAT_B5G6R5 == 1 );
    assert( GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 == 2 );
    assert( GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 == 3 );
    assert( colorFormat >= 0 && colorFormat < GPU_SURFACE_COLOR_FORMAT_MAX );

    const GpuSurfaceColorFormat* desiredFormat     = desiredFormatTable[colorFormat];
    const VkColorSpaceKHR        desiredColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED, then the surface has no preferred format.
    // Otherwise, at least one supported format will be returned.
    if ( formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED )
    {
        this->format         = colorFormat;
        this->internalFormat = InternalSurfaceColorFormat( desiredFormat[0] );
        this->colorSpace     = desiredColorSpace;
    }
    else
    {
        // Select the best matching surface format.
        assert( formatCount >= 1 );
        for ( uint32_t desired = 0; desired < GPU_SURFACE_COLOR_FORMAT_MAX; desired++ )
        {
            const VkFormat internalFormat = InternalSurfaceColorFormat( desiredFormat[desired] );
            for ( uint32_t available = 0; available < formatCount; available++ )
            {
                if ( surfaceFormats[available].format == internalFormat &&
                     surfaceFormats[available].colorSpace == desiredColorSpace )
                {
                    this->format         = desiredFormat[desired];
                    this->internalFormat = internalFormat;
                    this->colorSpace     = desiredColorSpace;
                    break;
                }
            }
            if ( this->internalFormat != VK_FORMAT_UNDEFINED )
            {
                break;
            }
        }
    }

    Print( "--------------------------------\n" );

    for ( uint32_t i = 0; i < formatCount; i++ )
    {
        const char* formatString = nullptr;
        switch ( surfaceFormats[i].format )
        {
            case VK_FORMAT_R5G6B5_UNORM_PACK16:
                formatString = "VK_FORMAT_R5G6B5_UNORM_PACK16";
                break;
            case VK_FORMAT_B5G6R5_UNORM_PACK16:
                formatString = "VK_FORMAT_B5G6R5_UNORM_PACK16";
                break;
            case VK_FORMAT_R8G8B8A8_UNORM:
                formatString = "VK_FORMAT_R8G8B8A8_UNORM";
                break;
            case VK_FORMAT_R8G8B8A8_SRGB:
                formatString = "VK_FORMAT_R8G8B8A8_SRGB";
                break;
            case VK_FORMAT_B8G8R8A8_UNORM:
                formatString = "VK_FORMAT_B8G8R8A8_UNORM";
                break;
            case VK_FORMAT_B8G8R8A8_SRGB:
                formatString = "VK_FORMAT_B8G8R8A8_SRGB";
                break;
            default:
            {
                static char number[32];
                sprintf( number, "%d", surfaceFormats[i].format );
                formatString = number;
                break;
            }
        }
        Print( "%s %s%s\n", ( i == 0 ? "Surface Formats      :" : "                      " ),
               formatString,
               ( this->internalFormat == surfaceFormats[i].format ) ? " (used)" : "" );
    }

    free( surfaceFormats );
    surfaceFormats = nullptr;

    // Check the surface proprties and formats.
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK( device->instance->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device->physicalDevice, surface, &surfaceCapabilities ) );

    assert( ( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) !=
            0 );
    assert( ( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT ) != 0 );
    //assert( ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR ) != 0 );

    uint32_t presentModeCount;
    VK( device->instance->vkGetPhysicalDeviceSurfacePresentModesKHR(
        device->physicalDevice, surface, &presentModeCount, nullptr ) );

    VkPresentModeKHR* presentModes =
        static_cast<VkPresentModeKHR*>(malloc( presentModeCount * sizeof( VkPresentModeKHR ) ));
    VK( device->instance->vkGetPhysicalDeviceSurfacePresentModesKHR(
        device->physicalDevice, surface, &presentModeCount, presentModes ) );

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkPresentModeKHR desiredPresendMode =
        ( ( swapInterval == 0 ) ? VK_PRESENT_MODE_IMMEDIATE_KHR
                                : ( ( swapInterval == -1 ) ? VK_PRESENT_MODE_FIFO_RELAXED_KHR
                                                           : VK_PRESENT_MODE_FIFO_KHR ) );
    if ( swapchainPresentMode != desiredPresendMode )
    {
        for ( uint32_t i = 0; i < presentModeCount; i++ )
        {
            if ( presentModes[i] == desiredPresendMode )
            {
                swapchainPresentMode = desiredPresendMode;
                break;
            }
        }
    }

    for ( uint32_t i = 0; i < presentModeCount; i++ )
    {
        const char* formatString = nullptr;
        switch ( presentModes[i] )
        {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                formatString = "VK_PRESENT_MODE_IMMEDIATE_KHR";
                break;
            case VK_PRESENT_MODE_MAILBOX_KHR:
                formatString = "VK_PRESENT_MODE_MAILBOX_KHR";
                break;
            case VK_PRESENT_MODE_FIFO_KHR:
                formatString = "VK_PRESENT_MODE_FIFO_KHR";
                break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                formatString = "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
                break;
            default:
            {
                static char number[32];
                sprintf( number, "%d", surfaceFormats[i].format );
                formatString = number;
                break;
            }
        }
        Print( "%s %s%s\n", ( i == 0 ? "Present Modes        :" : "                      " ),
               formatString, ( presentModes[i] == swapchainPresentMode ) ? " (used)" : "" );
    }

    free( presentModes );
    presentModes = nullptr;

    VkExtent2D swapchainExtent = {
        std::clamp( static_cast<uint32_t>(width), surfaceCapabilities.minImageExtent.width,
                    surfaceCapabilities.maxImageExtent.width ),
        std::clamp( static_cast<uint32_t>(height), surfaceCapabilities.minImageExtent.height,
                    surfaceCapabilities.maxImageExtent.height ),
    };

#if !defined( OS_ANDROID )
    // The width and height are either both -1, or both not -1.
    if ( surfaceCapabilities.currentExtent.width != (uint32_t)-1 )
    {
        // If the surface size is defined, the swapchain size must match.
        swapchainExtent = surfaceCapabilities.currentExtent;
    }
#endif

    this->width  = swapchainExtent.width;
    this->height = swapchainExtent.height;

    // Determine the number of VkImage's to use in the swapchain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapChainImages = surfaceCapabilities.minImageCount + 1;
    if ( ( surfaceCapabilities.maxImageCount > 0 ) &&
         ( desiredNumberOfSwapChainImages > surfaceCapabilities.maxImageCount ) )
    {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapChainImages = surfaceCapabilities.maxImageCount;
    }

    Print( "Swapchain Images     : %d\n", desiredNumberOfSwapChainImages );
    Print( "--------------------------------\n" );

    VkSurfaceTransformFlagBitsKHR preTransform;
    if ( surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = surfaceCapabilities.currentTransform;
    }

    const bool separatePresentQueue =
        ( device->presentQueueFamilyIndex != device->workQueueFamilyIndex );
    const uint32_t queueFamilyIndices[2] = { static_cast<uint32_t>( device->workQueueFamilyIndex ),
                                             static_cast<uint32_t>(
                                                 device->presentQueueFamilyIndex ) };

    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType              = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext              = nullptr;
    swapchainCreateInfo.flags              = 0;
    swapchainCreateInfo.surface            = surface;
    swapchainCreateInfo.minImageCount      = desiredNumberOfSwapChainImages;
    swapchainCreateInfo.imageFormat        = this->internalFormat;
    swapchainCreateInfo.imageColorSpace    = this->colorSpace;
    swapchainCreateInfo.imageExtent.width  = swapchainExtent.width;
    swapchainCreateInfo.imageExtent.height = swapchainExtent.height;
    swapchainCreateInfo.imageArrayLayers   = 1;
    swapchainCreateInfo.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    swapchainCreateInfo.imageSharingMode =
        separatePresentQueue ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = separatePresentQueue ? 2 : 0;
    swapchainCreateInfo.pQueueFamilyIndices   = separatePresentQueue ? queueFamilyIndices : nullptr;
    swapchainCreateInfo.preTransform          = preTransform;
    swapchainCreateInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode           = swapchainPresentMode;
    swapchainCreateInfo.clipped               = VK_TRUE;
    swapchainCreateInfo.oldSwapchain          = VK_NULL_HANDLE;

    VK( device->vkCreateSwapchainKHR( device->device, &swapchainCreateInfo, VK_ALLOCATOR,
                                      &this->swapchain ) );

    VK( device->vkGetSwapchainImagesKHR( device->device, this->swapchain, &this->imageCount,
                                         nullptr ) );

    this->images = static_cast<VkImage*>(malloc( this->imageCount * sizeof( VkImage ) ));
    VK( device->vkGetSwapchainImagesKHR( device->device, this->swapchain, &this->imageCount,
                                         this->images ) );

    this->views = static_cast<VkImageView*>(malloc( this->imageCount * sizeof( VkImageView ) ));

    for ( uint32_t i = 0; i < this->imageCount; i++ )
    {
        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext                       = nullptr;
        imageViewCreateInfo.flags                       = 0;
        imageViewCreateInfo.image                       = this->images[i];
        imageViewCreateInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format                      = this->internalFormat;
        imageViewCreateInfo.components.r                = VK_COMPONENT_SWIZZLE_R;
        imageViewCreateInfo.components.g                = VK_COMPONENT_SWIZZLE_G;
        imageViewCreateInfo.components.b                = VK_COMPONENT_SWIZZLE_B;
        imageViewCreateInfo.components.a                = VK_COMPONENT_SWIZZLE_A;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreateInfo.subresourceRange.levelCount     = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount     = 1;

        VK( device->vkCreateImageView( device->device, &imageViewCreateInfo, VK_ALLOCATOR,
                                       &this->views[i] ) );
    }

    this->bufferCount = this->imageCount;
    this->buffers     = static_cast<GpuSwapchainBuffer*>(
        malloc( this->bufferCount * sizeof( GpuSwapchainBuffer ) ) );
    for ( uint32_t i = 0; i < this->bufferCount; i++ )
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;

        this->buffers[i].imageIndex = 0;
        VK( device->vkCreateSemaphore( device->device, &semaphoreCreateInfo, VK_ALLOCATOR,
                                       &this->buffers[i].presentCompleteSemaphore ) );
        VK( device->vkCreateSemaphore( device->device, &semaphoreCreateInfo, VK_ALLOCATOR,
                                       &this->buffers[i].renderingCompleteSemaphore ) );
    }

    this->currentBuffer = 0;
    VK( device->vkAcquireNextImageKHR( device->device, this->swapchain, UINT64_MAX,
                                       this->buffers[this->currentBuffer].presentCompleteSemaphore,
                                       VK_NULL_HANDLE,
                                       &this->buffers[this->currentBuffer].imageIndex ) );

    VC( device->vkGetDeviceQueue( device->device, device->presentQueueFamilyIndex, 0,
                                  &this->presentQueue ) );

    assert( separatePresentQueue || this->presentQueue == context->queue );
}

GpuSwapchain::~GpuSwapchain() {}
} // namespace lxd
