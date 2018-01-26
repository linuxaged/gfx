#pragma once
#include "Gfx.hpp"
#include "nanoseconds.h"
#include "GpuContext.hpp"

namespace lxd
{
static const int GPU_TIMER_FRAMES_DELAYED = 2;

class GpuTimer
{
  public:
    GpuTimer( GpuContext* context );
    ~GpuTimer();
    ksNanoseconds GetNanoseconds() const;

    GpuContext&   context;
    VkBool32      supported{ 0 };
    ksNanoseconds period{ 0 };
    VkQueryPool   pool{ nullptr };
    uint32_t      init{ 0 };
    uint32_t      index{ 0 };
    uint64_t      data[2]{ 0 };
};
} // namespace lxd
