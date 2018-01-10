#pragma once

#include "Gfx.hpp"

namespace lxd
{

struct GpuFeature
{
    const char* name;
    bool        validationOnly;
    bool        required;
};

// Returns true if all the required features are present.
extern bool CheckFeatures( const char* label, const bool validationEnabled, const bool extensions,
                           const GpuFeature* requested, const uint32_t requestedCount,
                           const void* available, const uint32_t availableCount,
                           const char* enabledNames[], uint32_t* enabledCount );

class GpuInstance
{
  public:
    GpuInstance();
    ~GpuInstance();

  private:
    VkBool32   validate = false;
    void*      loader   = nullptr;
    VkInstance instance = nullptr;

    // Global functions.
    PFN_vkGetInstanceProcAddr                  vkGetInstanceProcAddr                  = nullptr;
    PFN_vkEnumerateInstanceLayerProperties     vkEnumerateInstanceLayerProperties     = nullptr;
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties = nullptr;
    PFN_vkCreateInstance                       vkCreateInstance                       = nullptr;

    // Instance functions.
    PFN_vkDestroyInstance                        vkDestroyInstance                        = nullptr;
    PFN_vkEnumeratePhysicalDevices               vkEnumeratePhysicalDevices               = nullptr;
    PFN_vkGetPhysicalDeviceFeatures              vkGetPhysicalDeviceFeatures              = nullptr;
    PFN_vkGetPhysicalDeviceProperties            vkGetPhysicalDeviceProperties            = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties      vkGetPhysicalDeviceMemoryProperties      = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties      vkGetPhysicalDeviceFormatProperties      = nullptr;
    PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties = nullptr;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties
                                             vkGetPhysicalDeviceSparseImageFormatProperties = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = nullptr;
    PFN_vkEnumerateDeviceLayerProperties     vkEnumerateDeviceLayerProperties     = nullptr;
    PFN_vkCreateDevice                       vkCreateDevice                       = nullptr;
    PFN_vkGetDeviceProcAddr                  vkGetDeviceProcAddr                  = nullptr;

    // Instance extensions.
    PFN_vkCreateSurfaceKHR                        vkCreateSurfaceKHR                   = nullptr;
    PFN_vkDestroySurfaceKHR                       vkDestroySurfaceKHR                  = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR      vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR =
        nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR =
        nullptr;

#if defined( OS_NEUTRAL_DISPLAY_SURFACE )
    PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR =
        nullptr;
    PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR = nullptr;
    PFN_vkGetDisplayModePropertiesKHR         vkGetDisplayModePropertiesKHR         = nullptr;
#endif

    // Debug callback.
    PFN_vkCreateDebugReportCallbackEXT  vkCreateDebugReportCallbackEXT  = nullptr;
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = nullptr;
    VkDebugReportCallbackEXT            debugReportCallback             = nullptr;
};

} // namespace lxd
