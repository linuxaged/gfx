#include <GpuContext.hpp>
#include "GpuDevice.hpp"
#include "threading.h"

namespace lxd
{
GpuContext::GpuContext( GpuDevice* device, const int queueIndex )
{
    ksMutex_Lock( &device->queueFamilyMutex, true );
    assert( ( device->queueFamilyUsedQueues[device->workQueueFamilyIndex] & ( 1 << queueIndex ) ) ==
            0 );
    device->queueFamilyUsedQueues[device->workQueueFamilyIndex] |= ( 1 << queueIndex );
    ksMutex_Unlock( &device->queueFamilyMutex );

    this->device           = device;
    this->queueFamilyIndex = device->workQueueFamilyIndex;
    this->queueIndex       = queueIndex;

    VC( device->vkGetDeviceQueue( device->device, this->queueFamilyIndex, this->queueIndex,
                                  &this->queue ) );

    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext            = nullptr;
    commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = this->queueFamilyIndex;

    VK( device->vkCreateCommandPool( device->device, &commandPoolCreateInfo, VK_ALLOCATOR,
                                     &this->commandPool ) );

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
    pipelineCacheCreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheCreateInfo.pNext           = nullptr;
    pipelineCacheCreateInfo.flags           = 0;
    pipelineCacheCreateInfo.initialDataSize = 0;
    pipelineCacheCreateInfo.pInitialData    = nullptr;

    VK( device->vkCreatePipelineCache( device->device, &pipelineCacheCreateInfo, VK_ALLOCATOR,
                                       &this->pipelineCache ) );
}

void GpuContext::CreateShared( const GpuContext* other, const int queueIndex )
{
    GpuContext( other->device, queueIndex );
}

GpuContext::~GpuContext()
{
    if ( this->device == nullptr )
    {
        return;
    }

    // Mark the queue as no longer in use.
    ksMutex_Lock( &this->device->queueFamilyMutex, true );
    assert( ( this->device->queueFamilyUsedQueues[this->queueFamilyIndex] &
              ( 1 << this->queueIndex ) ) != 0 );
    this->device->queueFamilyUsedQueues[this->queueFamilyIndex] &= ~( 1 << this->queueIndex );
    ksMutex_Unlock( &this->device->queueFamilyMutex );

    if ( this->setupCommandBuffer )
    {
        VC( this->device->vkFreeCommandBuffers( this->device->device, this->commandPool, 1,
                                                &this->setupCommandBuffer ) );
    }
    VC( this->device->vkDestroyCommandPool( this->device->device, this->commandPool,
                                            VK_ALLOCATOR ) );
    VC( this->device->vkDestroyPipelineCache( this->device->device, this->pipelineCache,
                                              VK_ALLOCATOR ) );
}

void GpuContext::WaitIdle() { VK( this->device->vkQueueWaitIdle( this->queue ) ); }

void GpuContext::GetLimits( GpuLimits* limits )
{
    limits->maxPushConstantsSize =
        this->device->physicalDeviceProperties.limits.maxPushConstantsSize;
    const VkSampleCountFlags availableSampleCounts =
        this->device->physicalDeviceProperties.limits.framebufferColorSampleCounts &
        this->device->physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    limits->maxSamples = 0;
    for ( int bit = VK_SAMPLE_COUNT_1_BIT; bit <= VK_SAMPLE_COUNT_64_BIT; bit <<= 1 )
    {
        if ( ( availableSampleCounts & bit ) == 0 )
        {
            break;
        }
        limits->maxSamples = bit;
    }
}

void GpuContext::CreateSetupCmdBuffer()
{
    if ( this->setupCommandBuffer != VK_NULL_HANDLE )
    {
        return;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext              = nullptr;
    commandBufferAllocateInfo.commandPool        = this->commandPool;
    commandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VK( this->device->vkAllocateCommandBuffers( this->device->device, &commandBufferAllocateInfo,
                                                &this->setupCommandBuffer ) );

    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext            = nullptr;
    commandBufferBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    VK( this->device->vkBeginCommandBuffer( this->setupCommandBuffer, &commandBufferBeginInfo ) );
}

void GpuContext::FlushSetupCmdBuffer()
{
    if ( this->setupCommandBuffer == VK_NULL_HANDLE )
    {
        return;
    }

    VK( this->device->vkEndCommandBuffer( this->setupCommandBuffer ) );

    VkSubmitInfo submitInfo;
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext                = nullptr;
    submitInfo.waitSemaphoreCount   = 0;
    submitInfo.pWaitSemaphores      = nullptr;
    submitInfo.pWaitDstStageMask    = nullptr;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &this->setupCommandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores    = nullptr;

    VK( this->device->vkQueueSubmit( this->queue, 1, &submitInfo, VK_NULL_HANDLE ) );
    VK( this->device->vkQueueWaitIdle( this->queue ) );

    VC( this->device->vkFreeCommandBuffers( this->device->device, this->commandPool, 1,
                                            &this->setupCommandBuffer ) );
    this->setupCommandBuffer = VK_NULL_HANDLE;
}
} // namespace lxd
