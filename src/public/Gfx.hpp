#pragma once

#include "SysUtils.hpp"

#if defined( OS_WINDOWS )

#    if !defined( _CRT_SECURE_NO_WARNINGS )
#        define _CRT_SECURE_NO_WARNINGS
#    endif

#    if defined( _MSC_VER )
#        pragma warning( disable : 4201 ) // nonstandard extension used: nameless struct/union
#        pragma warning(                                                                           \
            disable : 4204 ) // nonstandard extension used : non-constant aggregate initializer
#        pragma warning(                                                                           \
            disable : 4255 ) // '<name>' : no function prototype given: converting '()' to '(void)'
#        pragma warning(                                                                           \
            disable : 4668 ) // '__cplusplus' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#        pragma warning(                                                                           \
            disable : 4710 ) // 'int printf(const char *const ,...)': function not inlined
#        pragma warning(                                                                           \
            disable : 4711 ) // function '<name>' selected for automatic inline expansion
#        pragma warning(                                                                           \
            disable : 4738 ) // storing 32-bit float result in memory, possible loss of performance
#        pragma warning(                                                                           \
            disable : 4820 ) // '<name>' : 'X' bytes padding added after data member '<member>'
#    endif

#    if _MSC_VER >= 1900
#        pragma warning( disable : 4464 ) // relative include path contains '..'
#        pragma warning(                                                                           \
            disable : 4774 ) // 'printf' : format string expected in argument 1 is not a string literal
#    endif

#    include <windows.h>

#    define VK_USE_PLATFORM_WIN32_KHR
#    define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#    define PFN_vkCreateSurfaceKHR PFN_vkCreateWin32SurfaceKHR
#    define vkCreateSurfaceKHR vkCreateWin32SurfaceKHR

#    include "vk_format.h"
#    include "vulkan/vk_sdk_platform.h"
#    include "vulkan/vulkan.h"

#    define VULKAN_LOADER "vulkan-1.dll"
#    define OUTPUT_PATH ""

#    define __thread __declspec( thread )

#elif defined( OS_LINUX )

#    if defined( SUPPORT_X )
#        define OS_LINUX_XCB
#    endif
#    if __STDC_VERSION__ >= 199901L
#        define _XOPEN_SOURCE 600
#    else
#        define _XOPEN_SOURCE 500
#    endif

#    include <sys/time.h> // for gettimeofday()
#    include <time.h>     // for timespec
#    if !defined( __USE_UNIX98 )
#        define __USE_UNIX98 1 // for pthread_mutexattr_settype
#    endif
#    include <dlfcn.h>   // for dlopen
#    include <malloc.h>  // for memalign
#    include <pthread.h> // for pthread_create() etc.
#    if defined( OS_LINUX_XLIB )
#        include <X11/Xatom.h>
#        include <X11/Xlib.h>
#        include <X11/extensions/Xrandr.h>    // for resolution changes
#        include <X11/extensions/xf86vmode.h> // for fullscreen video mode
#    elif defined( OS_LINUX_XCB )
#        include <X11/keysym.h>
#        include <xcb/randr.h>
#        include <xcb/xcb.h>
#        include <xcb/xcb_icccm.h>
#        include <xcb/xcb_keysyms.h>
#    endif

#    if defined( OS_LINUX_XLIB )
#        define VK_USE_PLATFORM_XLIB_KHR
#        define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#        define PFN_vkCreateSurfaceKHR PFN_vkCreateXlibSurfaceKHR
#        define vkCreateSurfaceKHR vkCreateXlibSurfaceKHR
#    elif defined( OS_LINUX_XCB )
#        define VK_USE_PLATFORM_XCB_KHR
#        define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#        define PFN_vkCreateSurfaceKHR PFN_vkCreateXcbSurfaceKHR
#        define vkCreateSurfaceKHR vkCreateXcbSurfaceKHR
#    endif

#    include "vk_format.h"
#    include "vulkan/vk_sdk_platform.h"
#    include "vulkan/vulkan.h"

#    define VULKAN_LOADER "libvulkan.so"
#    define OUTPUT_PATH ""

// These prototypes are only included when __USE_GNU is defined but that causes other compile errors.
extern int pthread_setname_np( pthread_t __target_thread, __const char* __name );
extern int pthread_setaffinity_np( pthread_t thread, size_t cpusetsize, const cpu_set_t* cpuset );

#    pragma GCC diagnostic ignored "-Wunused-function"

#elif defined( OS_APPLE )

#    include <dlfcn.h> // for dlopen
#    include <pthread.h>
#    include <sys/param.h>
#    include <sys/sysctl.h>
#    include <sys/time.h>

    //#include "vulkan/vulkan.h"
    //#include "vulkan/vk_platform.h"
#    include "vk_format.h"

#    import <MoltenVK/mvk_vulkan.h>
#    include <MoltenVK/vulkan/vulkan.h>

#    if defined( OS_APPLE_IOS )
#        include <UIKit/UIKit.h>
        //#if 1
#        if defined( VK_USE_PLATFORM_IOS_MVK )
            //#define VK_USE_PLATFORM_IOS_MVK 1
#            include <MoltenVK/mvk_vulkan.h>
#            include <MoltenVK/vk_mvk_moltenvk.h>
#            include <QuartzCore/CAMetalLayer.h>
#            define VkIOSSurfaceCreateInfoKHR VkIOSSurfaceCreateInfoMVK
#            define VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_KHR                                  \
                VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK
#            define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_MVK_IOS_SURFACE_EXTENSION_NAME
#            define PFN_vkCreateSurfaceKHR PFN_vkCreateIOSSurfaceMVK
#            define vkCreateSurfaceKHR vkCreateIOSSurfaceMVK
#        else
#            error "mvk_vulkan no include"
// Only here to make the code compile.
typedef VkFlags VkIOSSurfaceCreateFlagsKHR;
typedef struct VkIOSSurfaceCreateInfoKHR
{
    VkStructureType            sType;
    const void*                pNext;
    VkIOSSurfaceCreateFlagsKHR flags;
    UIView*                    pView;
} VkIOSSurfaceCreateInfoKHR;
#            define VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_KHR 1000015000
#            define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME "VK_KHR_ios_surface"
typedef VkResult( VKAPI_PTR* PFN_vkCreateIOSSurfaceKHR )(
    VkInstance instance, const VkIOSSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface );
#            define PFN_vkCreateSurfaceKHR PFN_vkCreateIOSSurfaceKHR
#            define vkCreateSurfaceKHR vkCreateIOSSurfaceKHR
#            define VULKAN_LOADER "libvulkan.dylib"
#        endif
#    endif

#    if defined( OS_APPLE_MACOS )
#        include <AppKit/AppKit.h>
#        if defined( VK_USE_PLATFORM_MACOS_MVK )
#            include <MoltenVK/vk_mvk_macos_surface.h>
#            include <MoltenVK/vk_mvk_moltenvk.h>
#            include <QuartzCore/CAMetalLayer.h>
#            define VkMacOSSurfaceCreateInfoKHR VkMacOSSurfaceCreateInfoMVK
#            define VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_KHR                                \
                VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK
#            define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#            define PFN_vkCreateSurfaceKHR PFN_vkCreateMacOSSurfaceMVK
#            define vkCreateSurfaceKHR vkCreateMacOSSurfaceMVK
#        else
// Only here to make the code compile.
typedef VkFlags VkMacOSSurfaceCreateFlagsKHR;
typedef struct VkMacOSSurfaceCreateInfoKHR
{
    VkStructureType              sType;
    const void*                  pNext;
    VkMacOSSurfaceCreateFlagsKHR flags;
    NSView*                      pView;
} VkMacOSSurfaceCreateInfoKHR;
#            define VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_KHR 1000015000
#            define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME "VK_KHR_macos_surface"
typedef VkResult( VKAPI_PTR* PFN_vkCreateMacOSSurfaceKHR )(
    VkInstance instance, const VkMacOSSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface );
#            define PFN_vkCreateSurfaceKHR PFN_vkCreateMacOSSurfaceKHR
#            define vkCreateSurfaceKHR vkCreateMacOSSurfaceKHR
#            define VULKAN_LOADER "libvulkan.dylib"
#        endif
#    endif

#    undef MAX
#    undef MIN
#    define OUTPUT_PATH ""

#    pragma clang diagnostic ignored "-Wunused-function"
#    pragma clang diagnostic ignored "-Wunused-const-variable"

#elif defined( OS_ANDROID )

#    define VK_USE_PLATFORM_ANDROID_KHR
#    define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#    define PFN_vkCreateSurfaceKHR PFN_vkCreateAndroidSurfaceKHR
#    define vkCreateSurfaceKHR vkCreateAndroidSurfaceKHR

#    include "vulkan/vk_format.h"
#    include "vulkan/vk_sdk_platform.h"
#    include "vulkan/vulkan.h"

#    define VULKAN_LOADER "libvulkan.so"
#    define OUTPUT_PATH "/sdcard/"

#    pragma GCC diagnostic ignored "-Wunused-function"

#endif // OS_ANDROID

// fixme:
#if defined( OS_NEUTRAL_DISPLAY_SURFACE )
#    define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_DISPLAY_EXTENSION_NAME
#    define PFN_vkCreateSurfaceKHR PFN_vkCreateDisplayPlaneSurfaceKHR
#    define vkCreateSurfaceKHR vkCreateDisplayPlaneSurfaceKHR
#endif

#define VK_ALLOCATOR NULL

#define USE_VALIDATION 1
#define USE_SPIRV 1
#define USE_PM_MULTIVIEW 1
#define USE_API_DUMP                                                                               \
    0 // place vk_layer_settings.txt in the executable folder and change APIDumpFile = TRUE

#define ICD_SPV_MAGIC 0x07230203

#if USE_SPIRV == 1
#    define PROGRAM( name ) name##SPIRV
#else
#    define PROGRAM( name ) name##GLSL
#endif

#define SPIRV_VERSION "99"
#define GLSL_VERSION "440 core" // maintain precision decorations: "310 es"
#define GLSL_EXTENSIONS                                                                            \
    "#extension GL_EXT_shader_io_blocks : enable\n"                                                \
    "#extension GL_ARB_enhanced_layouts : enable\n"

#if !defined( VK_API_VERSION_1_0 )
#    define VK_API_VERSION_1_0 VK_API_VERSION
#endif

#if !defined( VK_VERSION_MAJOR )
#    define VK_VERSION_MAJOR( version ) ( ( uint32_t )( version ) >> 22 )
#    define VK_VERSION_MINOR( version ) ( ( ( uint32_t )( version ) >> 12 ) & 0x3ff )
#    define VK_VERSION_PATCH( version ) ( ( uint32_t )(version)&0xfff )
#endif

#define MAX_VERTEX_ATTRIBUTES 16

////////
//#if defined( _DEBUG )
//#    define VK( func )                                                                             \
//        VkCheckErrors( func, #func );                                                              \
//        ksFrameLog_Write( __FILE__, __LINE__, #func );
//#    define VC( func )                                                                             \
//        func;                                                                                      \
//        ksFrameLog_Write( __FILE__, __LINE__, #func );
//#else
#define VK( func ) VkCheckErrors( func, #func );
#define VC( func ) func;
//#endif

#define VK_ERROR_INVALID_SHADER_NV -1002

static const char* VkErrorString( VkResult result )
{
    switch ( result )
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        default:
        {
            if ( result == VK_ERROR_INVALID_SHADER_NV )
            {
                return "VK_ERROR_INVALID_SHADER_NV";
            }
            return "unknown";
        }
    }
}

static void VkCheckErrors( VkResult result, const char* function )
{
    if ( result != VK_SUCCESS )
    {
        Error( "Vulkan error: %s: %s\n", function, VkErrorString( result ) );
    }
}
