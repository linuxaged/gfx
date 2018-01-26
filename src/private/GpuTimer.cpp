#include "GpuTimer.hpp"
#include "GpuDevice.hpp"

namespace lxd
{
GpuTimer::GpuTimer( GpuContext* context ) : context( *context )
{
    this->supported =
        context->device->queueFamilyProperties[context->queueFamilyIndex].timestampValidBits != 0;
    if ( !this->supported )
    {
        return;
    }

    this->period = (ksNanoseconds)context->device->physicalDeviceProperties.limits.timestampPeriod;

    const uint32_t queryCount = ( GPU_TIMER_FRAMES_DELAYED + 1 ) * 2;

    VkQueryPoolCreateInfo queryPoolCreateInfo;
    queryPoolCreateInfo.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolCreateInfo.pNext              = NULL;
    queryPoolCreateInfo.flags              = 0;
    queryPoolCreateInfo.queryType          = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolCreateInfo.queryCount         = queryCount;
    queryPoolCreateInfo.pipelineStatistics = 0;

    VK( context->device->vkCreateQueryPool( context->device->device, &queryPoolCreateInfo,
                                            VK_ALLOCATOR, &this->pool ) );

    context->CreateSetupCmdBuffer();
    VC( context->device->vkCmdResetQueryPool( context->setupCommandBuffer, this->pool, 0,
                                              queryCount ) );
    context->FlushSetupCmdBuffer();
}

GpuTimer::~GpuTimer()
{
    if ( this->supported )
    {
        VC( context.device->vkDestroyQueryPool( context.device->device, this->pool,
                                                VK_ALLOCATOR ) );
    }
}

ksNanoseconds GpuTimer::GetNanoseconds() const
{
    if ( this->supported )
    {
        return ( this->data[1] - this->data[0] ) * this->period;
    }
    else
    {
        return 0;
    }
}
} // namespace lxd
