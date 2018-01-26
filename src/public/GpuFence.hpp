#pragma once

#include "Gfx.hpp"
#include "GpuContext.hpp"

namespace lxd
{
	class GpuFence
	{
	public:
		GpuFence(GpuContext* context);
		~GpuFence();

		void Submit();
		bool IsSignalled();

		GpuContext& context;
		VkFence fence;
		bool    submitted;
		
	};
}