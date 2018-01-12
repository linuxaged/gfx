#pragma once

#include "Gfx.hpp"
#include "threading.h"

namespace lxd
{

class GpuInstance;

enum GpuQueueProperty
{
    GPU_QUEUE_PROPERTY_GRAPHICS = 0b0,
    GPU_QUEUE_PROPERTY_COMPUTE  = 0b1,
    GPU_QUEUE_PROPERTY_TRANSFER = 0b10
};

enum GpuQueuePriority
{
    GPU_QUEUE_PRIORITY_LOW,
    GPU_QUEUE_PRIORITY_MEDIUM,
    GPU_QUEUE_PRIORITY_HIGH
};

static int const MAX_QUEUES = 16;
struct GpuQueueInfo
{
    int              queueCount;                  // number of queues
    GpuQueueProperty queueProperties;             // desired queue family properties
    GpuQueuePriority queuePriorities[MAX_QUEUES]; // individual queue priorities
};

class GpuDevice
{
  public:
    GpuDevice( GpuInstance* instance, GpuQueueInfo const* queueInfo,
               const VkSurfaceKHR presentSurface );
    ~GpuDevice();

    uint32_t GetMemoryTypeIndex( const uint32_t              typeBits,
                                 const VkMemoryPropertyFlags requiredProperties );

    void CreateShader( VkShaderModule* shaderModule, const VkShaderStageFlagBits stage,
                       const void* code, size_t codeSize );

  private:
    bool SelectPhysicalDevice( GpuInstance* instance, const GpuQueueInfo* queueInfo,
                               const VkSurfaceKHR presentSurface );

  public:
    VkBool32                         foundSwapchainExtension;
    GpuInstance*                     instance;
    uint32_t                         enabledExtensionCount;
    const char*                      enabledExtensionNames[32];
    uint32_t                         enabledLayerCount;
    const char*                      enabledLayerNames[32];
    VkPhysicalDevice                 physicalDevice;
    VkPhysicalDeviceFeatures         physicalDeviceFeatures;
    VkPhysicalDeviceProperties       physicalDeviceProperties;
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    uint32_t                         queueFamilyCount;
    VkQueueFamilyProperties*         queueFamilyProperties;
    uint32_t*                        queueFamilyUsedQueues;
    ksMutex                          queueFamilyMutex;
    int                              workQueueFamilyIndex;
    int                              presentQueueFamilyIndex;

    // The logical device.
    VkDevice device;

    // Device functions.
    PFN_vkDestroyDevice                    vkDestroyDevice;
    PFN_vkGetDeviceQueue                   vkGetDeviceQueue;
    PFN_vkQueueSubmit                      vkQueueSubmit;
    PFN_vkQueueWaitIdle                    vkQueueWaitIdle;
    PFN_vkDeviceWaitIdle                   vkDeviceWaitIdle;
    PFN_vkAllocateMemory                   vkAllocateMemory;
    PFN_vkFreeMemory                       vkFreeMemory;
    PFN_vkMapMemory                        vkMapMemory;
    PFN_vkUnmapMemory                      vkUnmapMemory;
    PFN_vkFlushMappedMemoryRanges          vkFlushMappedMemoryRanges;
    PFN_vkInvalidateMappedMemoryRanges     vkInvalidateMappedMemoryRanges;
    PFN_vkGetDeviceMemoryCommitment        vkGetDeviceMemoryCommitment;
    PFN_vkBindBufferMemory                 vkBindBufferMemory;
    PFN_vkBindImageMemory                  vkBindImageMemory;
    PFN_vkGetBufferMemoryRequirements      vkGetBufferMemoryRequirements;
    PFN_vkGetImageMemoryRequirements       vkGetImageMemoryRequirements;
    PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
    PFN_vkQueueBindSparse                  vkQueueBindSparse;
    PFN_vkCreateFence                      vkCreateFence;
    PFN_vkDestroyFence                     vkDestroyFence;
    PFN_vkResetFences                      vkResetFences;
    PFN_vkGetFenceStatus                   vkGetFenceStatus;
    PFN_vkWaitForFences                    vkWaitForFences;
    PFN_vkCreateSemaphore                  vkCreateSemaphore;
    PFN_vkDestroySemaphore                 vkDestroySemaphore;
    PFN_vkCreateEvent                      vkCreateEvent;
    PFN_vkDestroyEvent                     vkDestroyEvent;
    PFN_vkGetEventStatus                   vkGetEventStatus;
    PFN_vkSetEvent                         vkSetEvent;
    PFN_vkResetEvent                       vkResetEvent;
    PFN_vkCreateQueryPool                  vkCreateQueryPool;
    PFN_vkDestroyQueryPool                 vkDestroyQueryPool;
    PFN_vkGetQueryPoolResults              vkGetQueryPoolResults;
    PFN_vkCreateBuffer                     vkCreateBuffer;
    PFN_vkDestroyBuffer                    vkDestroyBuffer;
    PFN_vkCreateBufferView                 vkCreateBufferView;
    PFN_vkDestroyBufferView                vkDestroyBufferView;
    PFN_vkCreateImage                      vkCreateImage;
    PFN_vkDestroyImage                     vkDestroyImage;
    PFN_vkGetImageSubresourceLayout        vkGetImageSubresourceLayout;
    PFN_vkCreateImageView                  vkCreateImageView;
    PFN_vkDestroyImageView                 vkDestroyImageView;
    PFN_vkCreateShaderModule               vkCreateShaderModule;
    PFN_vkDestroyShaderModule              vkDestroyShaderModule;
    PFN_vkCreatePipelineCache              vkCreatePipelineCache;
    PFN_vkDestroyPipelineCache             vkDestroyPipelineCache;
    PFN_vkGetPipelineCacheData             vkGetPipelineCacheData;
    PFN_vkMergePipelineCaches              vkMergePipelineCaches;
    PFN_vkCreateGraphicsPipelines          vkCreateGraphicsPipelines;
    PFN_vkCreateComputePipelines           vkCreateComputePipelines;
    PFN_vkDestroyPipeline                  vkDestroyPipeline;
    PFN_vkCreatePipelineLayout             vkCreatePipelineLayout;
    PFN_vkDestroyPipelineLayout            vkDestroyPipelineLayout;
    PFN_vkCreateSampler                    vkCreateSampler;
    PFN_vkDestroySampler                   vkDestroySampler;
    PFN_vkCreateDescriptorSetLayout        vkCreateDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout       vkDestroyDescriptorSetLayout;
    PFN_vkCreateDescriptorPool             vkCreateDescriptorPool;
    PFN_vkDestroyDescriptorPool            vkDestroyDescriptorPool;
    PFN_vkResetDescriptorPool              vkResetDescriptorPool;
    PFN_vkAllocateDescriptorSets           vkAllocateDescriptorSets;
    PFN_vkFreeDescriptorSets               vkFreeDescriptorSets;
    PFN_vkUpdateDescriptorSets             vkUpdateDescriptorSets;
    PFN_vkCreateFramebuffer                vkCreateFramebuffer;
    PFN_vkDestroyFramebuffer               vkDestroyFramebuffer;
    PFN_vkCreateRenderPass                 vkCreateRenderPass;
    PFN_vkDestroyRenderPass                vkDestroyRenderPass;
    PFN_vkGetRenderAreaGranularity         vkGetRenderAreaGranularity;
    PFN_vkCreateCommandPool                vkCreateCommandPool;
    PFN_vkDestroyCommandPool               vkDestroyCommandPool;
    PFN_vkResetCommandPool                 vkResetCommandPool;
    PFN_vkAllocateCommandBuffers           vkAllocateCommandBuffers;
    PFN_vkFreeCommandBuffers               vkFreeCommandBuffers;
    PFN_vkBeginCommandBuffer               vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer                 vkEndCommandBuffer;
    PFN_vkResetCommandBuffer               vkResetCommandBuffer;
    PFN_vkCmdBindPipeline                  vkCmdBindPipeline;
    PFN_vkCmdSetViewport                   vkCmdSetViewport;
    PFN_vkCmdSetScissor                    vkCmdSetScissor;
    PFN_vkCmdSetLineWidth                  vkCmdSetLineWidth;
    PFN_vkCmdSetDepthBias                  vkCmdSetDepthBias;
    PFN_vkCmdSetBlendConstants             vkCmdSetBlendConstants;
    PFN_vkCmdSetDepthBounds                vkCmdSetDepthBounds;
    PFN_vkCmdSetStencilCompareMask         vkCmdSetStencilCompareMask;
    PFN_vkCmdSetStencilWriteMask           vkCmdSetStencilWriteMask;
    PFN_vkCmdSetStencilReference           vkCmdSetStencilReference;
    PFN_vkCmdBindDescriptorSets            vkCmdBindDescriptorSets;
    PFN_vkCmdBindIndexBuffer               vkCmdBindIndexBuffer;
    PFN_vkCmdBindVertexBuffers             vkCmdBindVertexBuffers;
    PFN_vkCmdDraw                          vkCmdDraw;
    PFN_vkCmdDrawIndexed                   vkCmdDrawIndexed;
    PFN_vkCmdDrawIndirect                  vkCmdDrawIndirect;
    PFN_vkCmdDrawIndexedIndirect           vkCmdDrawIndexedIndirect;
    PFN_vkCmdDispatch                      vkCmdDispatch;
    PFN_vkCmdDispatchIndirect              vkCmdDispatchIndirect;
    PFN_vkCmdCopyBuffer                    vkCmdCopyBuffer;
    PFN_vkCmdCopyImage                     vkCmdCopyImage;
    PFN_vkCmdBlitImage                     vkCmdBlitImage;
    PFN_vkCmdCopyBufferToImage             vkCmdCopyBufferToImage;
    PFN_vkCmdCopyImageToBuffer             vkCmdCopyImageToBuffer;
    PFN_vkCmdUpdateBuffer                  vkCmdUpdateBuffer;
    PFN_vkCmdFillBuffer                    vkCmdFillBuffer;
    PFN_vkCmdClearColorImage               vkCmdClearColorImage;
    PFN_vkCmdClearDepthStencilImage        vkCmdClearDepthStencilImage;
    PFN_vkCmdClearAttachments              vkCmdClearAttachments;
    PFN_vkCmdResolveImage                  vkCmdResolveImage;
    PFN_vkCmdSetEvent                      vkCmdSetEvent;
    PFN_vkCmdResetEvent                    vkCmdResetEvent;
    PFN_vkCmdWaitEvents                    vkCmdWaitEvents;
    PFN_vkCmdPipelineBarrier               vkCmdPipelineBarrier;
    PFN_vkCmdBeginQuery                    vkCmdBeginQuery;
    PFN_vkCmdEndQuery                      vkCmdEndQuery;
    PFN_vkCmdResetQueryPool                vkCmdResetQueryPool;
    PFN_vkCmdWriteTimestamp                vkCmdWriteTimestamp;
    PFN_vkCmdCopyQueryPoolResults          vkCmdCopyQueryPoolResults;
    PFN_vkCmdPushConstants                 vkCmdPushConstants;
    PFN_vkCmdBeginRenderPass               vkCmdBeginRenderPass;
    PFN_vkCmdNextSubpass                   vkCmdNextSubpass;
    PFN_vkCmdEndRenderPass                 vkCmdEndRenderPass;
    PFN_vkCmdExecuteCommands               vkCmdExecuteCommands;

    // Device extensions.
    PFN_vkCreateSwapchainKHR    vkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR   vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR   vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR       vkQueuePresentKHR;
};
} // namespace lxd
