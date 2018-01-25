#include "GpuDevice.hpp"
#include "GpuInstance.hpp"
#include <iterator>

namespace lxd
{

#define GET_DEVICE_PROC_ADDR_EXP( function )                                                       \
    this->function =                                                                               \
        ( PFN_##function )( this->instance->vkGetDeviceProcAddr( this->device, #function ) );      \
    assert( this->function != nullptr );
#define GET_DEVICE_PROC_ADDR( function ) GET_DEVICE_PROC_ADDR_EXP( function )

GpuDevice::GpuDevice( GpuInstance* instance, GpuQueueInfo const* queueInfo,
                      const VkSurfaceKHR presentSurface )
{
	bool result = SelectPhysicalDevice(instance, queueInfo, presentSurface);
	assert( result);

    //
    // Create the logical device
    //

    float floatPriorities[MAX_QUEUES];
    for ( int i = 0; i < queueInfo->queueCount; i++ )
    {
        const uint32_t discreteQueuePriorities =
            this->physicalDeviceProperties.limits.discreteQueuePriorities;
        switch ( queueInfo->queuePriorities[i] )
        {
            case GPU_QUEUE_PRIORITY_LOW:
                floatPriorities[i] = 0.0f;
                break;
            case GPU_QUEUE_PRIORITY_MEDIUM:
                floatPriorities[i] = ( discreteQueuePriorities <= 2 ) ? 0.0f : 0.5f;
                break;
            case GPU_QUEUE_PRIORITY_HIGH:
                floatPriorities[i] = 1.0f;
                break;
        }
    }

    // Create the device.
    VkDeviceQueueCreateInfo deviceQueueCreateInfo[2];
    deviceQueueCreateInfo[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo[0].pNext            = nullptr;
    deviceQueueCreateInfo[0].flags            = 0;
    deviceQueueCreateInfo[0].queueFamilyIndex = this->workQueueFamilyIndex;
    deviceQueueCreateInfo[0].queueCount       = queueInfo->queueCount;
    deviceQueueCreateInfo[0].pQueuePriorities = floatPriorities;

    deviceQueueCreateInfo[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo[1].pNext            = nullptr;
    deviceQueueCreateInfo[1].flags            = 0;
    deviceQueueCreateInfo[1].queueFamilyIndex = this->presentQueueFamilyIndex;
    deviceQueueCreateInfo[1].queueCount       = 1;
    deviceQueueCreateInfo[1].pQueuePriorities = nullptr;

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount =
        1 + ( this->presentQueueFamilyIndex != -1 &&
              this->presentQueueFamilyIndex != this->workQueueFamilyIndex );
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = this->enabledLayerCount;
    deviceCreateInfo.ppEnabledLayerNames =
        (const char* const*)( this->enabledLayerCount != 0 ? this->enabledLayerNames : nullptr );
    deviceCreateInfo.enabledExtensionCount = this->enabledExtensionCount;
    deviceCreateInfo.ppEnabledExtensionNames =
        (const char* const*)( this->enabledExtensionCount != 0 ? this->enabledExtensionNames
                                                               : nullptr );
    deviceCreateInfo.pEnabledFeatures = nullptr;

    VK( instance->vkCreateDevice( this->physicalDevice, &deviceCreateInfo, VK_ALLOCATOR,
                                  &this->device ) );

#if defined( VK_USE_PLATFORM_IOS_MVK ) || defined( VK_USE_PLATFORM_MACOS_MVK )
    // Specify some helpful MoltenVK extension configuration, such as performance logging.
    MVKDeviceConfiguration mvkConfig;
    vkGetMoltenVKDeviceConfigurationMVK( this->device, &mvkConfig );
    mvkConfig.performanceTracking = true;
    mvkConfig.performanceLoggingFrameCount =
        60; // Log once per second (actually once every 60 frames)
    vkSetMoltenVKDeviceConfigurationMVK( this->device, &mvkConfig );
#endif

    //
    // Setup the device specific function pointers
    //

    // Get the device functions.
    GET_DEVICE_PROC_ADDR( vkDestroyDevice );
    GET_DEVICE_PROC_ADDR( vkGetDeviceQueue );
    GET_DEVICE_PROC_ADDR( vkQueueSubmit );
    GET_DEVICE_PROC_ADDR( vkQueueWaitIdle );
    GET_DEVICE_PROC_ADDR( vkDeviceWaitIdle );
    GET_DEVICE_PROC_ADDR( vkAllocateMemory );
    GET_DEVICE_PROC_ADDR( vkFreeMemory );
    GET_DEVICE_PROC_ADDR( vkMapMemory );
    GET_DEVICE_PROC_ADDR( vkUnmapMemory );
    GET_DEVICE_PROC_ADDR( vkFlushMappedMemoryRanges );
    GET_DEVICE_PROC_ADDR( vkInvalidateMappedMemoryRanges );
    GET_DEVICE_PROC_ADDR( vkGetDeviceMemoryCommitment );
    GET_DEVICE_PROC_ADDR( vkBindBufferMemory );
    GET_DEVICE_PROC_ADDR( vkBindImageMemory );
    GET_DEVICE_PROC_ADDR( vkGetBufferMemoryRequirements );
    GET_DEVICE_PROC_ADDR( vkGetImageMemoryRequirements );
    GET_DEVICE_PROC_ADDR( vkGetImageSparseMemoryRequirements );
    GET_DEVICE_PROC_ADDR( vkQueueBindSparse );
    GET_DEVICE_PROC_ADDR( vkCreateFence );
    GET_DEVICE_PROC_ADDR( vkDestroyFence );
    GET_DEVICE_PROC_ADDR( vkResetFences );
    GET_DEVICE_PROC_ADDR( vkGetFenceStatus );
    GET_DEVICE_PROC_ADDR( vkWaitForFences );
    GET_DEVICE_PROC_ADDR( vkCreateSemaphore );
    GET_DEVICE_PROC_ADDR( vkDestroySemaphore );
    GET_DEVICE_PROC_ADDR( vkCreateEvent );
    GET_DEVICE_PROC_ADDR( vkDestroyEvent );
    GET_DEVICE_PROC_ADDR( vkGetEventStatus );
    GET_DEVICE_PROC_ADDR( vkSetEvent );
    GET_DEVICE_PROC_ADDR( vkResetEvent );
    GET_DEVICE_PROC_ADDR( vkCreateQueryPool );
    GET_DEVICE_PROC_ADDR( vkDestroyQueryPool );
    GET_DEVICE_PROC_ADDR( vkGetQueryPoolResults );
    GET_DEVICE_PROC_ADDR( vkCreateBuffer );
    GET_DEVICE_PROC_ADDR( vkDestroyBuffer );
    GET_DEVICE_PROC_ADDR( vkCreateBufferView );
    GET_DEVICE_PROC_ADDR( vkDestroyBufferView );
    GET_DEVICE_PROC_ADDR( vkCreateImage );
    GET_DEVICE_PROC_ADDR( vkDestroyImage );
    GET_DEVICE_PROC_ADDR( vkGetImageSubresourceLayout );
    GET_DEVICE_PROC_ADDR( vkCreateImageView );
    GET_DEVICE_PROC_ADDR( vkDestroyImageView );
    GET_DEVICE_PROC_ADDR( vkCreateShaderModule );
    GET_DEVICE_PROC_ADDR( vkDestroyShaderModule );
    GET_DEVICE_PROC_ADDR( vkCreatePipelineCache );
    GET_DEVICE_PROC_ADDR( vkDestroyPipelineCache );
    GET_DEVICE_PROC_ADDR( vkGetPipelineCacheData );
    GET_DEVICE_PROC_ADDR( vkMergePipelineCaches );
    GET_DEVICE_PROC_ADDR( vkCreateGraphicsPipelines );
    GET_DEVICE_PROC_ADDR( vkCreateComputePipelines );
    GET_DEVICE_PROC_ADDR( vkDestroyPipeline );
    GET_DEVICE_PROC_ADDR( vkCreatePipelineLayout );
    GET_DEVICE_PROC_ADDR( vkDestroyPipelineLayout );
    GET_DEVICE_PROC_ADDR( vkCreateSampler );
    GET_DEVICE_PROC_ADDR( vkDestroySampler );
    GET_DEVICE_PROC_ADDR( vkCreateDescriptorSetLayout );
    GET_DEVICE_PROC_ADDR( vkDestroyDescriptorSetLayout );
    GET_DEVICE_PROC_ADDR( vkCreateDescriptorPool );
    GET_DEVICE_PROC_ADDR( vkDestroyDescriptorPool );
    GET_DEVICE_PROC_ADDR( vkResetDescriptorPool );
    GET_DEVICE_PROC_ADDR( vkAllocateDescriptorSets );
    GET_DEVICE_PROC_ADDR( vkFreeDescriptorSets );
    GET_DEVICE_PROC_ADDR( vkUpdateDescriptorSets );
    GET_DEVICE_PROC_ADDR( vkCreateFramebuffer );
    GET_DEVICE_PROC_ADDR( vkDestroyFramebuffer );
    GET_DEVICE_PROC_ADDR( vkCreateRenderPass );
    GET_DEVICE_PROC_ADDR( vkDestroyRenderPass );
    GET_DEVICE_PROC_ADDR( vkGetRenderAreaGranularity );
    GET_DEVICE_PROC_ADDR( vkCreateCommandPool );
    GET_DEVICE_PROC_ADDR( vkDestroyCommandPool );
    GET_DEVICE_PROC_ADDR( vkResetCommandPool );
    GET_DEVICE_PROC_ADDR( vkAllocateCommandBuffers );
    GET_DEVICE_PROC_ADDR( vkFreeCommandBuffers );
    GET_DEVICE_PROC_ADDR( vkBeginCommandBuffer );
    GET_DEVICE_PROC_ADDR( vkEndCommandBuffer );
    GET_DEVICE_PROC_ADDR( vkResetCommandBuffer );
    GET_DEVICE_PROC_ADDR( vkCmdBindPipeline );
    GET_DEVICE_PROC_ADDR( vkCmdSetViewport );
    GET_DEVICE_PROC_ADDR( vkCmdSetScissor );
    GET_DEVICE_PROC_ADDR( vkCmdSetLineWidth );
    GET_DEVICE_PROC_ADDR( vkCmdSetDepthBias );
    GET_DEVICE_PROC_ADDR( vkCmdSetBlendConstants );
    GET_DEVICE_PROC_ADDR( vkCmdSetDepthBounds );
    GET_DEVICE_PROC_ADDR( vkCmdSetStencilCompareMask );
    GET_DEVICE_PROC_ADDR( vkCmdSetStencilWriteMask );
    GET_DEVICE_PROC_ADDR( vkCmdSetStencilReference );
    GET_DEVICE_PROC_ADDR( vkCmdBindDescriptorSets );
    GET_DEVICE_PROC_ADDR( vkCmdBindIndexBuffer );
    GET_DEVICE_PROC_ADDR( vkCmdBindVertexBuffers );
    GET_DEVICE_PROC_ADDR( vkCmdDraw );
    GET_DEVICE_PROC_ADDR( vkCmdDrawIndexed );
    GET_DEVICE_PROC_ADDR( vkCmdDrawIndirect );
    GET_DEVICE_PROC_ADDR( vkCmdDrawIndexedIndirect );
    GET_DEVICE_PROC_ADDR( vkCmdDispatch );
    GET_DEVICE_PROC_ADDR( vkCmdDispatchIndirect );
    GET_DEVICE_PROC_ADDR( vkCmdCopyBuffer );
    GET_DEVICE_PROC_ADDR( vkCmdCopyImage );
    GET_DEVICE_PROC_ADDR( vkCmdBlitImage );
    GET_DEVICE_PROC_ADDR( vkCmdCopyBufferToImage );
    GET_DEVICE_PROC_ADDR( vkCmdCopyImageToBuffer );
    GET_DEVICE_PROC_ADDR( vkCmdUpdateBuffer );
    GET_DEVICE_PROC_ADDR( vkCmdFillBuffer );
    GET_DEVICE_PROC_ADDR( vkCmdClearColorImage );
    GET_DEVICE_PROC_ADDR( vkCmdClearDepthStencilImage );
    GET_DEVICE_PROC_ADDR( vkCmdClearAttachments );
    GET_DEVICE_PROC_ADDR( vkCmdResolveImage );
    GET_DEVICE_PROC_ADDR( vkCmdSetEvent );
    GET_DEVICE_PROC_ADDR( vkCmdResetEvent );
    GET_DEVICE_PROC_ADDR( vkCmdWaitEvents );
    GET_DEVICE_PROC_ADDR( vkCmdPipelineBarrier );
    GET_DEVICE_PROC_ADDR( vkCmdBeginQuery );
    GET_DEVICE_PROC_ADDR( vkCmdEndQuery );
    GET_DEVICE_PROC_ADDR( vkCmdResetQueryPool );
    GET_DEVICE_PROC_ADDR( vkCmdWriteTimestamp );
    GET_DEVICE_PROC_ADDR( vkCmdCopyQueryPoolResults );
    GET_DEVICE_PROC_ADDR( vkCmdPushConstants );
    GET_DEVICE_PROC_ADDR( vkCmdBeginRenderPass );
    GET_DEVICE_PROC_ADDR( vkCmdNextSubpass );
    GET_DEVICE_PROC_ADDR( vkCmdEndRenderPass );
    GET_DEVICE_PROC_ADDR( vkCmdExecuteCommands );

    // Get the swapchain extension functions.
    if ( this->foundSwapchainExtension )
    {
        GET_DEVICE_PROC_ADDR( vkCreateSwapchainKHR );
        GET_DEVICE_PROC_ADDR( vkCreateSwapchainKHR );
        GET_DEVICE_PROC_ADDR( vkDestroySwapchainKHR );
        GET_DEVICE_PROC_ADDR( vkGetSwapchainImagesKHR );
        GET_DEVICE_PROC_ADDR( vkAcquireNextImageKHR );
        GET_DEVICE_PROC_ADDR( vkQueuePresentKHR );
    }
}
GpuDevice::~GpuDevice()
{
    VK( this->vkDeviceWaitIdle( this->device ) );

    free( this->queueFamilyProperties );
    free( this->queueFamilyUsedQueues );

    ksMutex_Destroy( &this->queueFamilyMutex );

    VC( this->vkDestroyDevice( this->device, VK_ALLOCATOR ) );
}

bool GpuDevice::SelectPhysicalDevice( GpuInstance* instance, const GpuQueueInfo* queueInfo,
                                      const VkSurfaceKHR presentSurface )
{
    this->instance = instance;

    //
    // Select an appropriate physical device
    //

    const VkQueueFlags requiredQueueFlags =
        ( ( ( queueInfo->queueProperties & GPU_QUEUE_PROPERTY_GRAPHICS ) != 0 )
              ? VK_QUEUE_GRAPHICS_BIT
              : 0 ) |
        ( ( ( queueInfo->queueProperties & GPU_QUEUE_PROPERTY_COMPUTE ) != 0 )
              ? VK_QUEUE_COMPUTE_BIT
              : 0 ) |
        ( ( ( queueInfo->queueProperties & GPU_QUEUE_PROPERTY_TRANSFER ) != 0 &&
            ( queueInfo->queueProperties &
              ( GPU_QUEUE_PROPERTY_GRAPHICS | GPU_QUEUE_PROPERTY_COMPUTE ) ) == 0 )
              ? VK_QUEUE_TRANSFER_BIT
              : 0 );

    uint32_t physicalDeviceCount = 0;
    VK( instance->vkEnumeratePhysicalDevices( instance->instance, &physicalDeviceCount, nullptr ) );

    VkPhysicalDevice* physicalDevices =
        (VkPhysicalDevice*)malloc( physicalDeviceCount * sizeof( VkPhysicalDevice ) );
    VK( instance->vkEnumeratePhysicalDevices( instance->instance, &physicalDeviceCount,
                                              physicalDevices ) );

    for ( uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount;
          physicalDeviceIndex++ )
    {
        // Get the device properties.
        VkPhysicalDeviceProperties physicalDeviceProperties;
        VC( instance->vkGetPhysicalDeviceProperties( physicalDevices[physicalDeviceIndex],
                                                     &physicalDeviceProperties ) );

        const uint32_t driverMajor = VK_VERSION_MAJOR( physicalDeviceProperties.driverVersion );
        const uint32_t driverMinor = VK_VERSION_MINOR( physicalDeviceProperties.driverVersion );
        const uint32_t driverPatch = VK_VERSION_PATCH( physicalDeviceProperties.driverVersion );

        const uint32_t apiMajor = VK_VERSION_MAJOR( physicalDeviceProperties.apiVersion );
        const uint32_t apiMinor = VK_VERSION_MINOR( physicalDeviceProperties.apiVersion );
        const uint32_t apiPatch = VK_VERSION_PATCH( physicalDeviceProperties.apiVersion );

        Print( "--------------------------------\n" );
        Print( "Device Name          : %s\n", physicalDeviceProperties.deviceName );
        Print( "Device Type          : %s\n",
               ( ( physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU )
                     ? "integrated GPU"
                     : ( ( physicalDeviceProperties.deviceType ==
                           VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
                             ? "discrete GPU"
                             : ( ( physicalDeviceProperties.deviceType ==
                                   VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU )
                                     ? "virtual GPU"
                                     : ( ( physicalDeviceProperties.deviceType ==
                                           VK_PHYSICAL_DEVICE_TYPE_CPU )
                                             ? "CPU"
                                             : "unknown" ) ) ) ) );
        Print( "Vendor ID            : 0x%04X\n",
               physicalDeviceProperties.vendorID ); // http://pcidatabase.com
        Print( "Device ID            : 0x%04X\n", physicalDeviceProperties.deviceID );
        Print( "Driver Version       : %d.%d.%d\n", driverMajor, driverMinor, driverPatch );
        Print( "API Version          : %d.%d.%d\n", apiMajor, apiMinor, apiPatch );

        // Get the queue families.
        uint32_t queueFamilyCount = 0;
        VC( instance->vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevices[physicalDeviceIndex], &queueFamilyCount, nullptr ) );

        VkQueueFamilyProperties* queueFamilyProperties = (VkQueueFamilyProperties*)malloc(
            queueFamilyCount * sizeof( VkQueueFamilyProperties ) );
        VC( instance->vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevices[physicalDeviceIndex], &queueFamilyCount, queueFamilyProperties ) );

        for ( uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount;
              queueFamilyIndex++ )
        {
            const VkQueueFlags queueFlags = queueFamilyProperties[queueFamilyIndex].queueFlags;
            Print( "%-21s%c %d =%s%s%s (%d queues, %d priorities)\n",
                   ( queueFamilyIndex == 0 ? "Queue Families" : "" ),
                   ( queueFamilyIndex == 0 ? ':' : ' ' ), queueFamilyIndex,
                   ( queueFlags & VK_QUEUE_GRAPHICS_BIT ) ? " graphics" : "",
                   ( queueFlags & VK_QUEUE_COMPUTE_BIT ) ? " compute" : "",
                   ( queueFlags & VK_QUEUE_TRANSFER_BIT ) ? " transfer" : "",
                   queueFamilyProperties[queueFamilyIndex].queueCount,
                   physicalDeviceProperties.limits.discreteQueuePriorities );
        }

        // Check if this physical device supports the required queue families.
        int workQueueFamilyIndex    = -1;
        int presentQueueFamilyIndex = -1;
        for ( uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount;
              queueFamilyIndex++ )
        {
            if ( ( queueFamilyProperties[queueFamilyIndex].queueFlags & requiredQueueFlags ) ==
                 requiredQueueFlags )
            {
                if ( static_cast<int>(queueFamilyProperties[queueFamilyIndex].queueCount) >=
                     queueInfo->queueCount )
                {
                    workQueueFamilyIndex = queueFamilyIndex;
                }
            }
            if ( presentSurface != VK_NULL_HANDLE )
            {
                VkBool32 supportsPresent = VK_FALSE;
                VK( instance->vkGetPhysicalDeviceSurfaceSupportKHR(
                    physicalDevices[physicalDeviceIndex], queueFamilyIndex, presentSurface,
                    &supportsPresent ) );
                if ( supportsPresent )
                {
                    presentQueueFamilyIndex = queueFamilyIndex;
                }
            }
            if ( workQueueFamilyIndex != -1 &&
                 ( presentQueueFamilyIndex != -1 || presentSurface == VK_NULL_HANDLE ) )
            {
                break;
            }
        }
#if defined( OS_ANDROID )
        // On Android all devices must be able to present to the system compositor, and all queue families
        // must support the necessary image layout transitions and synchronization operations.
        presentQueueFamilyIndex = workQueueFamilyIndex;
#endif

        if ( workQueueFamilyIndex == -1 )
        {
            Print( "Required work queue family not supported.\n" );
            free( queueFamilyProperties );
            continue;
        }

        if ( presentQueueFamilyIndex == -1 && presentSurface != VK_NULL_HANDLE )
        {
            Print( "Required present queue family not supported.\n" );
            free( queueFamilyProperties );
            continue;
        }

        Print( "Work Queue Family    : %d\n", workQueueFamilyIndex );
        Print( "Present Queue Family : %d\n", presentQueueFamilyIndex );

        const GpuFeature requestedExtensions[] = {
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME, false, true },
            { "VK_NV_glsl_shader", false, false },
        };

        // Check the device extensions.
        bool requiedExtensionsAvailable = true;
        {
            uint32_t availableExtensionCount = 0;
            VK( instance->vkEnumerateDeviceExtensionProperties(
                physicalDevices[physicalDeviceIndex], nullptr, &availableExtensionCount,
                nullptr ) );

            VkExtensionProperties* availableExtensions = static_cast<VkExtensionProperties*>(malloc(
                availableExtensionCount * sizeof( VkExtensionProperties ) ));
            VK( instance->vkEnumerateDeviceExtensionProperties(
                physicalDevices[physicalDeviceIndex], nullptr, &availableExtensionCount,
                availableExtensions ) );

            requiedExtensionsAvailable =
                CheckFeatures( "Device Extensions", instance->validate, true, requestedExtensions,
                               static_cast<uint32_t>( std::size( requestedExtensions ) ),
                               availableExtensions, availableExtensionCount,
                               this->enabledExtensionNames, &this->enabledExtensionCount );

            free( availableExtensions );
        }

        if ( !requiedExtensionsAvailable )
        {
            Print( "Required device extensions not supported.\n" );
            free( queueFamilyProperties );
            continue;
        }

        // Check the device layers.
        const GpuFeature requestedLayers[] = {
            { "VK_LAYER_OCULUS_glsl_shader", false, false },
            { "VK_LAYER_OCULUS_queue_muxer", false, false },
            { "VK_LAYER_GOOGLE_threading", true, false },
            { "VK_LAYER_LUNARG_parameter_validation", true, false },
            { "VK_LAYER_LUNARG_object_tracker", true, false },
            { "VK_LAYER_LUNARG_core_validation", true, false },
            { "VK_LAYER_LUNARG_device_limits", true, false },
            { "VK_LAYER_LUNARG_image", true, false },
            { "VK_LAYER_LUNARG_swapchain", true, false },
            { "VK_LAYER_GOOGLE_unique_objects", true, false },
#if USE_API_DUMP == 1
            { "VK_LAYER_LUNARG_api_dump", true, false },
#endif
        };

        bool requiredLayersAvailable = true;
        {
            uint32_t availableLayerCount = 0;
            VK( instance->vkEnumerateDeviceLayerProperties( physicalDevices[physicalDeviceIndex],
                                                            &availableLayerCount, nullptr ) );

            VkLayerProperties* availableLayers =
                static_cast<VkLayerProperties*>(malloc( availableLayerCount * sizeof( VkLayerProperties ) ));
            VK( instance->vkEnumerateDeviceLayerProperties(
                physicalDevices[physicalDeviceIndex], &availableLayerCount, availableLayers ) );

            requiredLayersAvailable = CheckFeatures(
                "Device Layers", instance->validate, false, requestedLayers,
                static_cast<uint32_t>( std::size( requestedLayers ) ), availableLayers,
                availableLayerCount, this->enabledLayerNames, &this->enabledLayerCount );

            free( availableLayers );
        }

        if ( !requiredLayersAvailable )
        {
            Print( "Required device layers not supported.\n" );
            free( queueFamilyProperties );
            continue;
        }

        this->foundSwapchainExtension = requiredLayersAvailable;
        this->physicalDevice          = physicalDevices[physicalDeviceIndex];
        this->queueFamilyCount        = queueFamilyCount;
        this->queueFamilyProperties   = queueFamilyProperties;
        this->workQueueFamilyIndex    = workQueueFamilyIndex;
        this->presentQueueFamilyIndex = presentQueueFamilyIndex;

        VC( instance->vkGetPhysicalDeviceFeatures( physicalDevices[physicalDeviceIndex],
                                                   &this->physicalDeviceFeatures ) );
        VC( instance->vkGetPhysicalDeviceProperties( physicalDevices[physicalDeviceIndex],
                                                     &this->physicalDeviceProperties ) );
        VC( instance->vkGetPhysicalDeviceMemoryProperties(
            physicalDevices[physicalDeviceIndex], &this->physicalDeviceMemoryProperties ) );
        Print( "Support compressed texture formats:\nETC: %s\nASTC: %s\nBC: %s\n",
               this->physicalDeviceFeatures.textureCompressionETC2 ? "true" : "false",
               this->physicalDeviceFeatures.textureCompressionASTC_LDR ? "true" : "false",
               this->physicalDeviceFeatures.textureCompressionBC ? "true" : "false" );
        break;
    }

    Print( "--------------------------------\n" );

    if ( this->physicalDevice == VK_NULL_HANDLE )
    {
        Error( "No capable Vulkan physical device found." );
        return false;
    }

    // Allocate a bit mask for the available queues per family.
    this->queueFamilyUsedQueues = static_cast<uint32_t*>(malloc( this->queueFamilyCount * sizeof( uint32_t ) ));
    for ( uint32_t queueFamilyIndex = 0; queueFamilyIndex < this->queueFamilyCount;
          queueFamilyIndex++ )
    {
        this->queueFamilyUsedQueues[queueFamilyIndex] =
            0xFFFFFFFF << this->queueFamilyProperties[queueFamilyIndex].queueCount;
    }

    ksMutex_Create( &this->queueFamilyMutex );

    return true;
}

uint32_t GpuDevice::GetMemoryTypeIndex( const uint32_t              typeBits,
                                        const VkMemoryPropertyFlags requiredProperties )
{
    // Search memory types to find the index with the requested properties.
    for ( uint32_t type = 0; type < this->physicalDeviceMemoryProperties.memoryTypeCount; type++ )
    {
        if ( ( typeBits & ( 1 << type ) ) != 0 )
        {
            // Test if this memory type has the required properties.
            const VkFlags propertyFlags =
                this->physicalDeviceMemoryProperties.memoryTypes[type].propertyFlags;
            if ( ( propertyFlags & requiredProperties ) == requiredProperties )
            {
                return type;
            }
        }
    }
    Error( "Memory type %d with properties %d not found.", typeBits, requiredProperties );
    return 0;
}

void GpuDevice::CreateShader( VkShaderModule* shaderModule, const VkShaderStageFlagBits stage,
                              const void* code, size_t codeSize )
{
    UNUSED_PARM( stage );

    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext    = nullptr;
    moduleCreateInfo.flags    = 0;
    moduleCreateInfo.codeSize = 0;
    moduleCreateInfo.pCode    = nullptr;

    if ( *(uint32_t*)code == ICD_SPV_MAGIC )
    {
        moduleCreateInfo.codeSize = codeSize;
        moduleCreateInfo.pCode    = static_cast<const uint32_t*>( code );

        VK( this->vkCreateShaderModule( this->device, &moduleCreateInfo, VK_ALLOCATOR,
                                        shaderModule ) );
    }
    else
    {
        // Create fake SPV structure to feed GLSL to the driver "under the covers".
        size_t    tempCodeSize = 3 * sizeof( uint32_t ) + codeSize + 1;
        uint32_t* tempCode     = (uint32_t*)malloc( tempCodeSize );
        tempCode[0]            = ICD_SPV_MAGIC;
        tempCode[1]            = 0;
        tempCode[2]            = stage;
        memcpy( tempCode + 3, code, codeSize + 1 );

        moduleCreateInfo.codeSize = tempCodeSize;
        moduleCreateInfo.pCode    = tempCode;

        VK( this->vkCreateShaderModule( this->device, &moduleCreateInfo, VK_ALLOCATOR,
                                        shaderModule ) );

        free( tempCode );
    }
}

} // namespace lxd
