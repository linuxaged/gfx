#pragma once

#include "Gfx.hpp"
#include "GpuGraphicsCommand.hpp"
#include "GpuComputeCommand.hpp"
#include "GpuTexture.hpp"

namespace lxd
{
enum GpuBufferUnmapType
{
    GPU_BUFFER_UNMAP_TYPE_USE_ALLOCATED, // use the newly allocated (host visible) buffer
    GPU_BUFFER_UNMAP_TYPE_COPY_BACK      // copy back to the original buffer
};

enum GpuCommandBufferType
{
    GPU_COMMAND_BUFFER_TYPE_PRIMARY,
    GPU_COMMAND_BUFFER_TYPE_SECONDARY,
    GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS
};

static const int MAX_COMMAND_BUFFER_TIMERS = 16;

class GpuContext;
class GpuFence;
class GpuBuffer;
class GpuPipelineResources;
class GpuSwapchainBuffer;
class GpuFramebuffer;
class GpuRenderPass;
class GpuTimer;
class GpuGeometry;
struct ScreenRect;
class GpuVertexAttributeArrays;

class GpuCommandBuffer
{
  public:
    GpuCommandBuffer();
    ~GpuCommandBuffer();

	void BeginPrimary();
	void EndPrimary();
	GpuFence * SubmitPrimary();

	void ChangeTextureUsage(GpuTexture * texture, const GpuTextureUsage usage);

	void BeginFramebuffer(GpuFramebuffer * framebuffer, const int arrayLayer, const GpuTextureUsage usage);
	void EndFramebuffer(GpuFramebuffer * framebuffer, const int arrayLayer, const GpuTextureUsage usage);

	void BeginTimer(GpuTimer * timer);
	void EndTimer(GpuTimer * timer);

	void BeginRenderPass(GpuRenderPass * renderPass, GpuFramebuffer * framebuffer, const ScreenRect * rect);
	void EndRenderPass(GpuRenderPass * renderPass);

	void SetViewport(const ScreenRect * rect);
	void SetScissor(const ScreenRect * rect);

	void SubmitGraphicsCommand(const GpuGraphicsCommand * command);
	void SubmitComputeCommand(const GpuComputeCommand * command);

	GpuBuffer * MapBuffer(GpuBuffer * buffer, void ** data);
	void UnmapBuffer(GpuBuffer * buffer, GpuBuffer * mappedBuffer, const GpuBufferUnmapType type);

	GpuBuffer * MapVertexAttributes(GpuGeometry * geometry, GpuVertexAttributeArrays * attribs);
	void UnmapVertexAttributes(GpuGeometry * geometry, GpuBuffer * mappedVertexBuffer, const GpuBufferUnmapType type);

	GpuBuffer * MapInstanceAttributes(GpuGeometry * geometry, GpuVertexAttributeArrays * attribs);
	void UnmapInstanceAttributes(GpuGeometry * geometry, GpuBuffer * mappedInstanceBuffer, const GpuBufferUnmapType type);

public:
	GpuCommandBufferType   type = {};
	int                    numBuffers = {};
	int                    currentBuffer = {};
	VkCommandBuffer*       cmdBuffers = {};
	GpuContext*            context = {};
	GpuFence*              fences = {};
	GpuBuffer**            mappedBuffers = {};
	GpuBuffer**            oldMappedBuffers = {};
	GpuPipelineResources** pipelineResources = {};
	GpuSwapchainBuffer*    swapchainBuffer = {};
	GpuGraphicsCommand     currentGraphicsState = {};
	GpuComputeCommand      currentComputeState = {};
	GpuFramebuffer*        currentFramebuffer = {};
	GpuRenderPass*         currentRenderPass = {};
	GpuTimer*              currentTimers[MAX_COMMAND_BUFFER_TIMERS] = {};
	int                    currentTimerCount = {};
};
} // namespace lxd
