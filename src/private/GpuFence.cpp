#include "GpuFence.hpp"
#include "GpuDevice.hpp"
namespace lxd
{
GpuFence::GpuFence( GpuContext* context ) : context( *context )
{
    VkFenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = NULL;
    fenceCreateInfo.flags = 0;

    VK( context->device->vkCreateFence( context->device->device, &fenceCreateInfo, VK_ALLOCATOR,
                                        &this->fence ) );

    this->submitted = false;
}

GpuFence::~GpuFence()
{
    VC( context.device->vkDestroyFence( context.device->device, this->fence, VK_ALLOCATOR ) );
    this->fence     = VK_NULL_HANDLE;
    this->submitted = false;
}

void GpuFence::Submit() { this->submitted = true; }

bool GpuFence::IsSignalled()
{
    if ( fence == NULL || !this->submitted )
    {
        return false;
    }
    VC( VkResult res = context.device->vkGetFenceStatus( context.device->device, this->fence ) );
    return ( res == VK_SUCCESS );
}
} // namespace lxd
