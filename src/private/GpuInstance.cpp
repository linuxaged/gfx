#include "GpuInstance.hpp"
#include "SysUtils.hpp"
#include <iterator>

namespace lxd
{

// Match strings except for hexadecimal numbers.
static bool MatchStrings( const char* str1, const char* str2 )
{
    while ( str1[0] != '\0' && str2[0] != '\0' )
    {
        if ( str1[0] != str2[0] )
        {
            for ( ; str1[0] == 'x' || ( str1[0] >= '0' && str1[0] <= '9' ) ||
                    ( str1[0] >= 'a' && str1[0] <= 'f' ) || ( str1[0] >= 'A' && str1[0] <= 'F' );
                  str1++ )
            {
            }
            for ( ; str2[0] == 'x' || ( str2[0] >= '0' && str2[0] <= '9' ) ||
                    ( str2[0] >= 'a' && str2[0] <= 'f' ) || ( str2[0] >= 'A' && str2[0] <= 'F' );
                  str2++ )
            {
            }
            if ( str1[0] != str2[0] )
            {
                return false;
            }
        }
        str1++;
        str2++;
    }
    return true;
}

VkBool32 DebugReportCallback( VkDebugReportFlagsEXT                       msgFlags,
                              [[maybe_unused]] VkDebugReportObjectTypeEXT objType,
                              [[maybe_unused]] uint64_t srcObject, [[maybe_unused]] size_t location,
                              int32_t msgCode, const char* pLayerPrefix, const char* pMsg,
                              [[maybe_unused]] void* pUserData )
{
    // This performance warning is valid but this is how the the secondary command buffer is used.
    // [DS] "vkBeginCommandBuffer(): Secondary Command Buffers (00000039460DB2F8) may perform better if a valid framebuffer parameter is specified."
    if ( MatchStrings( pMsg,
                       "vkBeginCommandBuffer(): Secondary Command Buffers (00000039460DB2F8) may "
                       "perform better if a valid framebuffer parameter is specified." ) )
    {
        return VK_FALSE;
    }

    // [DS] "Cannot get query results on queryPool 0x1b3 with index 4 which is in flight."
    if ( MatchStrings(
             pMsg,
             "Cannot get query results on queryPool 0x1b3 with index 4 which is in flight." ) )
    {
        return VK_FALSE;
    }

    const bool warning = ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) == 0 ? true : false;
    Error( "%s: [%s] Code %d : %s", warning ? "Warning" : "Error", pLayerPrefix, msgCode, pMsg );
    return VK_FALSE;
}

#define GET_INSTANCE_PROC_ADDR_EXP( function )                                                     \
    this->function =                                                                               \
        ( PFN_##function )( this->vkGetInstanceProcAddr( this->instance, #function ) );            \
    assert( this->function != nullptr );
#define GET_INSTANCE_PROC_ADDR( function ) GET_INSTANCE_PROC_ADDR_EXP( function )

// Returns true if all the required features are present.
bool CheckFeatures( const char* label, const bool validationEnabled, const bool extensions,
                    const GpuFeature* requested, const uint32_t requestedCount,
                    const void* available, const uint32_t availableCount,
                    const char* enabledNames[], uint32_t* enabledCount )
{
    bool foundAllRequired = true;
    *enabledCount         = 0;
    for ( uint32_t i = 0; i < requestedCount; i++ )
    {
        bool        found  = false;
        const char* result = requested[i].required ? "(required, not found)" : "(not found)";
        for ( uint32_t j = 0; j < availableCount; j++ )
        {
            const char* name = extensions
                                   ? ( static_cast<const VkExtensionProperties*>(available) )[j].extensionName
                                   : ( static_cast<const VkLayerProperties*>(available) )[j].layerName;
            if ( strcmp( requested[i].name, name ) == 0 )
            {
                found = true;
                if ( requested[i].validationOnly && !validationEnabled )
                {
                    result = "(not enabled)";
                    break;
                }
                enabledNames[( *enabledCount )++] = requested[i].name;
                result = requested[i].required ? "(required, enabled)" : "(enabled)";
                break;
            }
        }
        foundAllRequired &= ( found || !requested[i].required );
        Print( "%-21s%c %s %s\n", ( i == 0 ? label : "" ), ( i == 0 ? ':' : ' ' ),
               requested[i].name, result );
    }
    return foundAllRequired;
}

GpuInstance::GpuInstance()
{
#if defined( _DEBUG ) && USE_VALIDATION == 1
    this->validate = VK_TRUE;
#else
    this->validate = VK_FALSE;
#endif

    // Get the global functions.
#if defined( OS_WINDOWS )
    this->loader = LoadLibrary( TEXT( VULKAN_LOADER ) );
    if ( this->loader == nullptr )
    {
        char buffer[1024];
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, nullptr, GetLastError(),
                       MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ),
                       nullptr );
        Error( "%s not available: %s", VULKAN_LOADER, buffer );
    }
    this->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(
        static_cast<HMODULE>( this->loader ), "vkGetInstanceProcAddr" );
    this->vkEnumerateInstanceLayerProperties =
        (PFN_vkEnumerateInstanceLayerProperties)GetProcAddress(
            static_cast<HMODULE>( this->loader ), "vkEnumerateInstanceLayerProperties" );
    this->vkEnumerateInstanceExtensionProperties =
        (PFN_vkEnumerateInstanceExtensionProperties)GetProcAddress(
            static_cast<HMODULE>( this->loader ), "vkEnumerateInstanceExtensionProperties" );
    this->vkCreateInstance = (PFN_vkCreateInstance)GetProcAddress(
        static_cast<HMODULE>( this->loader ), "vkCreateInstance" );
#elif defined( OS_LINUX ) || defined( OS_ANDROID ) ||                                              \
    ( defined( OS_APPLE ) && !defined( VK_USE_PLATFORM_IOS_MVK ) &&                                \
      !defined( VK_USE_PLATFORM_MACOS_MVK ) )
    this->loader   = dlopen( VULKAN_LOADER, RTLD_NOW | RTLD_LOCAL );
    if ( this->loader == nullptr )
    {
        Error( "%s not available: %s", VULKAN_LOADER, dlerror() );
        return false;
    }
    this->vkGetInstanceProcAddr =
        (PFN_vkGetInstanceProcAddr)dlsym( this->loader, "vkGetInstanceProcAddr" );
    this->vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)dlsym(
        this->loader, "vkEnumerateInstanceLayerProperties" );
    this->vkEnumerateInstanceExtensionProperties =
        (PFN_vkEnumerateInstanceExtensionProperties)dlsym(
            this->loader, "vkEnumerateInstanceExtensionProperties" );
    this->vkCreateInstance = (PFN_vkCreateInstance)dlsym( this->loader, "vkCreateInstance" );
#else
    this->vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    GET_INSTANCE_PROC_ADDR( vkEnumerateInstanceLayerProperties );
    GET_INSTANCE_PROC_ADDR( vkEnumerateInstanceExtensionProperties );
    GET_INSTANCE_PROC_ADDR( vkCreateInstance );
#endif

    Print( "--------------------------------\n" );

    // Get the instance extensions.
    const GpuFeature requestedExtensions[] = {
        { VK_KHR_SURFACE_EXTENSION_NAME, false, true },
#if defined( OS_NEUTRAL_DISPLAY_SURFACE )
        { VK_KHR_DISPLAY_EXTENSION_NAME, false, true },
#endif
        { VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME, false, true },
        { VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true, false },
    };

    const char* enabledExtensionNames[32]  = { nullptr };
    uint32_t    enabledExtensionCount      = 0;
    bool        requiedExtensionsAvailable = true;
    {
        uint32_t availableExtensionCount = 0;
        VK( this->vkEnumerateInstanceExtensionProperties( nullptr, &availableExtensionCount, nullptr ) );

        VkExtensionProperties* availableExtensions = static_cast<VkExtensionProperties*>(malloc(
            availableExtensionCount * sizeof( VkExtensionProperties ) ));
        VK( this->vkEnumerateInstanceExtensionProperties( nullptr, &availableExtensionCount,
                                                          availableExtensions ) );

        requiedExtensionsAvailable = CheckFeatures(
            "Instance Extensions", this->validate, true, requestedExtensions,
            static_cast<uint32_t>( std::size( requestedExtensions ) ), availableExtensions,
            availableExtensionCount, enabledExtensionNames, &enabledExtensionCount );

        free( availableExtensions );
    }
    if ( !requiedExtensionsAvailable )
    {
        Print( "Required instance extensions not supported.\n" );
    }

    // Get the instance layers.
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

    const char* enabledLayerNames[32]   = { 0 };
    uint32_t    enabledLayerCount       = 0;
    bool        requiredLayersAvailable = true;
    {
        uint32_t availableLayerCount = 0;
        VK( this->vkEnumerateInstanceLayerProperties( &availableLayerCount, nullptr ) );

        VkLayerProperties* availableLayers =
            (VkLayerProperties*)malloc( availableLayerCount * sizeof( VkLayerProperties ) );
        VK( this->vkEnumerateInstanceLayerProperties( &availableLayerCount, availableLayers ) );

        requiredLayersAvailable =
            CheckFeatures( "Instance Layers", this->validate, false, requestedLayers,
                           static_cast<uint32_t>( std::size( requestedLayers ) ), availableLayers,
                           availableLayerCount, enabledLayerNames, &enabledLayerCount );

        free( availableLayers );
    }
    if ( !requiredLayersAvailable )
    {
        Print( "Required instance layers not supported.\n" );
    }

    const uint32_t apiMajor = VK_VERSION_MAJOR( VK_API_VERSION_1_0 );
    const uint32_t apiMinor = VK_VERSION_MINOR( VK_API_VERSION_1_0 );
    const uint32_t apiPatch = VK_VERSION_PATCH( VK_API_VERSION_1_0 );
    Print( "Instance API version : %d.%d.%d\n", apiMajor, apiMinor, apiPatch );

    Print( "--------------------------------\n" );

    // Create the instance.
    VkApplicationInfo app;
    app.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext              = nullptr;
    app.pApplicationName   = "gfx";
    app.applicationVersion = 0;
    app.pEngineName        = "gfx";
    app.engineVersion      = 0;
    app.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.sType             = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext             = nullptr;
    instanceCreateInfo.flags             = 0;
    instanceCreateInfo.pApplicationInfo  = &app;
    instanceCreateInfo.enabledLayerCount = enabledLayerCount;
    instanceCreateInfo.ppEnabledLayerNames =
        (const char* const*)( enabledLayerCount != 0 ? enabledLayerNames : nullptr );
    instanceCreateInfo.enabledExtensionCount = enabledExtensionCount;
    instanceCreateInfo.ppEnabledExtensionNames =
        (const char* const*)( enabledExtensionCount != 0 ? enabledExtensionNames : nullptr );

    VK( this->vkCreateInstance( &instanceCreateInfo, VK_ALLOCATOR, &this->instance ) );

    // Get the instance functions.
    GET_INSTANCE_PROC_ADDR( vkDestroyInstance );
    GET_INSTANCE_PROC_ADDR( vkEnumeratePhysicalDevices );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceFeatures );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceProperties );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceMemoryProperties );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceQueueFamilyProperties );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceFormatProperties );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceImageFormatProperties );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSparseImageFormatProperties );
    GET_INSTANCE_PROC_ADDR( vkEnumerateDeviceExtensionProperties );
    GET_INSTANCE_PROC_ADDR( vkEnumerateDeviceLayerProperties );
    GET_INSTANCE_PROC_ADDR( vkCreateDevice );
    GET_INSTANCE_PROC_ADDR( vkGetDeviceProcAddr );

#if defined( OS_NEUTRAL_DISPLAY_SURFACE )
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceDisplayPropertiesKHR )
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceDisplayPlanePropertiesKHR )
    GET_INSTANCE_PROC_ADDR( vkGetDisplayPlaneSupportedDisplaysKHR )
    GET_INSTANCE_PROC_ADDR( vkGetDisplayModePropertiesKHR )
#endif

    // Get the surface extension functions.
    GET_INSTANCE_PROC_ADDR( vkCreateSurfaceKHR );
    GET_INSTANCE_PROC_ADDR( vkDestroySurfaceKHR );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSurfaceSupportKHR );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSurfaceCapabilitiesKHR );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSurfaceFormatsKHR );
    GET_INSTANCE_PROC_ADDR( vkGetPhysicalDeviceSurfacePresentModesKHR );

    if ( this->validate )
    {
        GET_INSTANCE_PROC_ADDR( vkCreateDebugReportCallbackEXT );
        GET_INSTANCE_PROC_ADDR( vkDestroyDebugReportCallbackEXT );
        if ( this->vkCreateDebugReportCallbackEXT != nullptr )
        {
            VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo;
            debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
            debugReportCallbackCreateInfo.pNext = nullptr;
            debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                                  VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                                  VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            //VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
            //VK_DEBUG_REPORT_DEBUG_BIT_EXT;
            debugReportCallbackCreateInfo.pfnCallback =
                (PFN_vkDebugReportCallbackEXT)DebugReportCallback;
            debugReportCallbackCreateInfo.pUserData = nullptr;

            VK( this->vkCreateDebugReportCallbackEXT( this->instance,
                                                      &debugReportCallbackCreateInfo, VK_ALLOCATOR,
                                                      &this->debugReportCallback ) );
        }
    }
}

GpuInstance::~GpuInstance()
{
    if ( this->validate && this->vkDestroyDebugReportCallbackEXT != nullptr )
    {
        VC( this->vkDestroyDebugReportCallbackEXT( this->instance, this->debugReportCallback,
                                                   VK_ALLOCATOR ) );
    }

    VC( this->vkDestroyInstance( this->instance, VK_ALLOCATOR ) );

    if ( this->loader != nullptr )
    {
#if defined( OS_WINDOWS )
        FreeLibrary( static_cast<HMODULE>( this->loader ) );
#elif defined( OS_LINUX ) || defined( OS_ANDROID )
        dlclose( this->loader );
#endif
    }
}

} // namespace lxd
