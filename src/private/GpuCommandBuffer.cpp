#include "GpuCommandBuffer.hpp"

namespace lxd
{
GpuCommandBuffer::GpuCommandBuffer()
{
    this->type          = type;
    this->numBuffers    = numBuffers;
    this->currentBuffer = 0;
    this->context       = context;
    this->cmdBuffers = (VkCommandBuffer*)malloc( numBuffers * sizeof( VkCommandBuffer ) );
    this->fences     = (GpuFence*)malloc( numBuffers * sizeof( GpuFence ) );
    this->mappedBuffers    = (GpuBuffer**)malloc( numBuffers * sizeof( GpuBuffer* ) );
    this->oldMappedBuffers = (GpuBuffer**)malloc( numBuffers * sizeof( GpuBuffer* ) );
    this->pipelineResources =
        (GpuPipelineResources**)malloc( numBuffers * sizeof( GpuPipelineResources* ) );

    for ( int i = 0; i < numBuffers; i++ )
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo;
        commandBufferAllocateInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext       = NULL;
        commandBufferAllocateInfo.commandPool = context->commandPool;
        commandBufferAllocateInfo.level       = ( type == GPU_COMMAND_BUFFER_TYPE_PRIMARY )
                                              ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                                              : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        VK( context->device->vkAllocateCommandBuffers(
            context->device->device, &commandBufferAllocateInfo, &this->cmdBuffers[i] ) );

        GpuFence_Create( context, &this->fences[i] );

        this->mappedBuffers[i]     = NULL;
        this->oldMappedBuffers[i]  = NULL;
        this->pipelineResources[i] = NULL;
    }
}

GpuCommandBuffer::~GpuCommandBuffer()
{
    assert( context == this->context );

    for ( int i = 0; i < this->numBuffers; i++ )
    {
        VC( context->device->vkFreeCommandBuffers( context->device->device, context->commandPool, 1,
                                                   &this->cmdBuffers[i] ) );

        GpuFence_Destroy( context, &this->fences[i] );

        for ( GpuBuffer *b = this->mappedBuffers[i], *next = NULL; b != NULL; b = next )
        {
            next = b->next;
            GpuBuffer_Destroy( context, b );
            free( b );
        }
        this->mappedBuffers[i] = NULL;

        for ( GpuBuffer *b = this->oldMappedBuffers[i], *next = NULL; b != NULL;
              b = next )
        {
            next = b->next;
            GpuBuffer_Destroy( context, b );
            free( b );
        }
        this->oldMappedBuffers[i] = NULL;

        for ( GpuPipelineResources *r = this->pipelineResources[i], *next = NULL;
              r != NULL; r = next )
        {
            next = r->next;
            GpuPipelineResources_Destroy( context, r );
            free( r );
        }
        this->pipelineResources[i] = NULL;
    }

    free( this->pipelineResources );
    free( this->oldMappedBuffers );
    free( this->mappedBuffers );
    free( this->fences );
    free( this->cmdBuffers );
}

void      GpuCommandBuffer::BeginPrimary() {
	assert(this->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY);
	assert(this->currentFramebuffer == NULL);
	assert(this->currentRenderPass == NULL);

	GpuDevice* device = this->context->device;

	this->currentBuffer = (this->currentBuffer + 1) % this->numBuffers;

	GpuFence* fence = &this->fences[this->currentBuffer];
	if (fence->submitted)
	{
		VK(device->vkWaitForFences(device->device, 1, &fence->fence, VK_TRUE,
			1ULL * 1000 * 1000 * 1000));
		VK(device->vkResetFences(device->device, 1, &fence->fence));
		fence->submitted = false;
	}

	GpuCommandBuffer_ManageBuffers(commandBuffer);

	GpuGraphicsCommand_Init(&this->currentGraphicsState);
	GpuComputeCommand_Init(&this->currentComputeState);

	VK(device->vkResetCommandBuffer(this->cmdBuffers[this->currentBuffer],
		0));

	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = NULL;
	commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pInheritanceInfo = NULL;

	VK(device->vkBeginCommandBuffer(this->cmdBuffers[this->currentBuffer],
		&commandBufferBeginInfo));

	// Make sure any CPU writes are flushed.
	{
		VkMemoryBarrier memoryBarrier;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = NULL;
		memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		memoryBarrier.dstAccessMask = 0;

		const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_HOST_BIT;
		const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkDependencyFlags    flags = 0;

		VC(device->vkCmdPipelineBarrier(this->cmdBuffers[this->currentBuffer],
			src_stages, dst_stages, flags, 1, &memoryBarrier, 0, NULL,
			0, NULL));
	}

}
void      GpuCommandBuffer::EndPrimary() {
	assert(this->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY);
	assert(this->currentFramebuffer == NULL);
	assert(this->currentRenderPass == NULL);

	GpuCommandBuffer_ManageTimers(commandBuffer);

	GpuDevice* device = this->context->device;
	VK(device->vkEndCommandBuffer(this->cmdBuffers[this->currentBuffer]));
}
GpuFence* GpuCommandBuffer::SubmitPrimary() {
	assert(this->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY);
	assert(this->currentFramebuffer == NULL);
	assert(this->currentRenderPass == NULL);

	GpuDevice* device = this->context->device;

	const VkPipelineStageFlags stageFlags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = (this->swapchainBuffer != NULL) ? 1 : 0;
	submitInfo.pWaitSemaphores = (this->swapchainBuffer != NULL)
		? &this->swapchainBuffer->presentCompleteSemaphore
		: NULL;
	submitInfo.pWaitDstStageMask = (this->swapchainBuffer != NULL) ? stageFlags : NULL;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->cmdBuffers[this->currentBuffer];
	submitInfo.signalSemaphoreCount = (this->swapchainBuffer != NULL) ? 1 : 0;
	submitInfo.pSignalSemaphores = (this->swapchainBuffer != NULL)
		? &this->swapchainBuffer->renderingCompleteSemaphore
		: NULL;

	GpuFence* fence = &this->fences[this->currentBuffer];
	VK(device->vkQueueSubmit(this->context->queue, 1, &submitInfo, fence->fence));
	GpuFence_Submit(this->context, fence);

	this->swapchainBuffer = NULL;

	return fence;
}

void GpuCommandBuffer::ChangeTextureUsage( GpuTexture* texture, const GpuTextureUsage usage ) {

}

void GpuCommandBuffer::BeginFramebuffer( GpuFramebuffer* framebuffer, const int arrayLayer,
                                         const GpuTextureUsage usage )
{
	assert(this->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY);
	assert(this->currentFramebuffer == NULL);
	assert(this->currentRenderPass == NULL);
	assert(arrayLayer >= 0 && arrayLayer < framebuffer->numLayers);

	if (framebuffer->window != NULL)
	{
		assert(framebuffer->window->swapchain.swapchain != VK_NULL_HANDLE);
		if (framebuffer->swapchainCreateCount != framebuffer->window->swapchainCreateCount)
		{
			GpuWindow*     window = framebuffer->window;
			GpuRenderPass* renderPass = framebuffer->renderPass;
			GpuFramebuffer_Destroy(this->context, framebuffer);
			GpuFramebuffer_CreateFromSwapchain(window, framebuffer, renderPass);
		}

		// Keep track of the current swapchain buffer to handle the swapchain semaphores.
		assert(this->swapchainBuffer == NULL);
		this->swapchainBuffer =
			&framebuffer->window->swapchain.buffers[framebuffer->window->swapchain.currentBuffer];

		framebuffer->currentBuffer = this->swapchainBuffer->imageIndex;
		framebuffer->currentLayer = 0;
	}
	else
	{
		// Only advance when rendering to the first layer.
		if (arrayLayer == 0)
		{
			framebuffer->currentBuffer =
				(framebuffer->currentBuffer + 1) % framebuffer->numBuffers;
		}
		framebuffer->currentLayer = arrayLayer;
	}

	assert(framebuffer->depthBuffer.internalFormat == VK_FORMAT_UNDEFINED ||
		framebuffer->depthBuffer.imageLayout ==
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	GpuCommandBuffer_ChangeTextureUsage(
		commandBuffer, &framebuffer->colorTextures[framebuffer->currentBuffer], usage);

	this->currentFramebuffer = framebuffer;
}
void GpuCommandBuffer::EndFramebuffer( GpuFramebuffer* framebuffer, const int arrayLayer,
                                       const GpuTextureUsage usage )
{
	assert(this->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY);
	assert(this->currentFramebuffer == framebuffer);
	assert(this->currentRenderPass == NULL);
	assert(arrayLayer >= 0 && arrayLayer < framebuffer->numLayers);

	UNUSED_PARM(arrayLayer);

#if EXPLICIT_RESOLVE != 0
	if (framebuffer->renderTexture.image != VK_NULL_HANDLE)
	{
		GpuCommandBuffer_ChangeTextureUsage(commandBuffer, &framebuffer->renderTexture,
			GPU_TEXTURE_USAGE_TRANSFER_SRC);
		GpuCommandBuffer_ChangeTextureUsage(
			commandBuffer, &framebuffer->colorTextures[framebuffer->currentBuffer],
			GPU_TEXTURE_USAGE_TRANSFER_DST);

		VkImageResolve region;
		region.srcOffset.x = 0;
		region.srcOffset.y = 0;
		region.srcOffset.z = 0;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = arrayLayer;
		region.srcSubresource.layerCount = 1;
		region.dstOffset.x = 0;
		region.dstOffset.y = 0;
		region.dstOffset.z = 0;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = arrayLayer;
		region.dstSubresource.layerCount = 1;
		region.extent.width = framebuffer->renderTexture.width;
		region.extent.height = framebuffer->renderTexture.height;
		region.extent.depth = framebuffer->renderTexture.depth;

		this->context->device->vkCmdResolveImage(
			this->cmdBuffers[this->currentBuffer],
			framebuffer->renderTexture.image, framebuffer->renderTexture.imageLayout,
			framebuffer->colorTextures[framebuffer->currentBuffer].image,
			framebuffer->colorTextures[framebuffer->currentBuffer].imageLayout, 1, &region);

		GpuCommandBuffer_ChangeTextureUsage(commandBuffer, &framebuffer->renderTexture,
			GPU_TEXTURE_USAGE_COLOR_ATTACHMENT);
	}
#endif

	GpuCommandBuffer_ChangeTextureUsage(
		commandBuffer, &framebuffer->colorTextures[framebuffer->currentBuffer], usage);

	this->currentFramebuffer = NULL;
}

void GpuCommandBuffer::BeginTimer( GpuTimer* timer ) {
	GpuDevice* device = this->context->device;

	if (!timer->supported)
	{
		return;
	}

	// Make sure this timer has not already been used.
	for (int i = 0; i < this->currentTimerCount; i++)
	{
		assert(this->currentTimers[i] != timer);
	}

	VC(device->vkCmdWriteTimestamp(this->cmdBuffers[this->currentBuffer],
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, timer->pool,
		timer->index * 2 + 0));
}
void GpuCommandBuffer::EndTimer( GpuTimer* timer ) {
	GpuDevice* device = this->context->device;

	if (!timer->supported)
	{
		return;
	}

	VC(device->vkCmdWriteTimestamp(this->cmdBuffers[this->currentBuffer],
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, timer->pool,
		timer->index * 2 + 1));

	assert(this->currentTimerCount < MAX_COMMAND_BUFFER_TIMERS);
	this->currentTimers[this->currentTimerCount++] = timer;
}

void GpuCommandBuffer::BeginRenderPass( GpuRenderPass* renderPass, GpuFramebuffer* framebuffer,
                                        const ScreenRect* rect )
{
	assert(this->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY);
	assert(this->currentRenderPass == NULL);
	assert(this->currentFramebuffer == framebuffer);

	GpuDevice* device = this->context->device;

	VkCommandBuffer cmdBuffer = this->cmdBuffers[this->currentBuffer];

	uint32_t     clearValueCount = 0;
	VkClearValue clearValues[3];
	memset(clearValues, 0, sizeof(clearValues));

	if (renderPass->sampleCount > GPU_SAMPLE_COUNT_1)
	{
		clearValues[clearValueCount].color.float32[0] = 0.0f;
		clearValues[clearValueCount].color.float32[1] = 0.0f;
		clearValues[clearValueCount].color.float32[2] = 0.0f;
		clearValues[clearValueCount].color.float32[3] = 1.0f;
		clearValueCount++;
	}
	if (renderPass->sampleCount <= GPU_SAMPLE_COUNT_1 || EXPLICIT_RESOLVE == 0)
	{
		clearValues[clearValueCount].color.float32[0] = 0.0f;
		clearValues[clearValueCount].color.float32[1] = 0.0f;
		clearValues[clearValueCount].color.float32[2] = 0.0f;
		clearValues[clearValueCount].color.float32[3] = 1.0f;
		clearValueCount++;
	}
	if (renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED)
	{
		clearValues[clearValueCount].depthStencil.depth = 1.0f;
		clearValues[clearValueCount].depthStencil.stencil = 0;
		clearValueCount++;
	}

	VkRenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = NULL;
	renderPassBeginInfo.renderPass = renderPass->renderPass;
	renderPassBeginInfo.framebuffer =
		framebuffer->framebuffers[framebuffer->currentBuffer * framebuffer->numLayers +
		framebuffer->currentLayer];
	renderPassBeginInfo.renderArea.offset.x = rect->x;
	renderPassBeginInfo.renderArea.offset.y = rect->y;
	renderPassBeginInfo.renderArea.extent.width = rect->width;
	renderPassBeginInfo.renderArea.extent.height = rect->height;
	renderPassBeginInfo.clearValueCount = clearValueCount;
	renderPassBeginInfo.pClearValues = clearValues;

	VkSubpassContents contents = (renderPass->type == GPU_RENDERPASS_TYPE_INLINE)
		? VK_SUBPASS_CONTENTS_INLINE
		: VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

	VC(device->vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, contents));

	this->currentRenderPass = renderPass;
}
void GpuCommandBuffer::EndRenderPass( GpuRenderPass* renderPass ) {
	assert(this->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY);
	assert(this->currentRenderPass == renderPass);

	UNUSED_PARM(renderPass);

	GpuDevice* device = this->context->device;

	VkCommandBuffer cmdBuffer = this->cmdBuffers[this->currentBuffer];

	VC(device->vkCmdEndRenderPass(cmdBuffer));

	this->currentRenderPass = NULL;
}

void GpuCommandBuffer::SetViewport( const ScreenRect* rect ) {
	GpuDevice* device = this->context->device;

	VkViewport viewport;
	viewport.x = (float)rect->x;
	viewport.y = (float)rect->y;
	viewport.width = (float)rect->width;
	viewport.height = (float)rect->height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkCommandBuffer cmdBuffer = this->cmdBuffers[this->currentBuffer];
	VC(device->vkCmdSetViewport(cmdBuffer, 0, 1, &viewport));
}
void GpuCommandBuffer::SetScissor( const ScreenRect* rect ) {
	GpuDevice* device = this->context->device;

	VkRect2D scissor;
	scissor.offset.x = rect->x;
	scissor.offset.y = rect->y;
	scissor.extent.width = rect->width;
	scissor.extent.height = rect->height;

	VkCommandBuffer cmdBuffer = this->cmdBuffers[this->currentBuffer];
	VC(device->vkCmdSetScissor(cmdBuffer, 0, 1, &scissor));
}

void GpuCommandBuffer::SubmitGraphicsCommand( const GpuGraphicsCommand* command ) {

	assert(this->currentRenderPass != NULL);

	GpuDevice* device = this->context->device;

	VkCommandBuffer             cmdBuffer = this->cmdBuffers[this->currentBuffer];
	const GpuGraphicsCommand* state = &this->currentGraphicsState;

	// If the pipeline has changed.
	if (command->pipeline != state->pipeline)
	{
		VC(device->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			command->pipeline->pipeline));
	}

	const GpuProgramParmLayout* commandLayout = &command->pipeline->program->parmLayout;
	const GpuProgramParmLayout* stateLayout =
		(state->pipeline != NULL) ? &state->pipeline->program->parmLayout : NULL;

	GpuCommandBuffer_UpdateProgramParms(commandBuffer, commandLayout, stateLayout,
		&command->parmState, &state->parmState,
		VK_PIPELINE_BIND_POINT_GRAPHICS);

	const GpuGeometry* geometry = command->pipeline->geometry;

	// If the geometry has changed.
	if (state->pipeline == NULL || geometry != state->pipeline->geometry ||
		command->vertexBuffer != state->vertexBuffer ||
		command->instanceBuffer != state->instanceBuffer)
	{
		const VkBuffer vertexBuffer = (command->vertexBuffer != NULL)
			? command->vertexBuffer->buffer
			: geometry->vertexBuffer.buffer;
		for (int i = 0; i < command->pipeline->firstInstanceBinding; i++)
		{
			VC(device->vkCmdBindVertexBuffers(cmdBuffer, i, 1, &vertexBuffer,
				&command->pipeline->vertexBindingOffsets[i]));
		}

		const VkBuffer instanceBuffer = (command->instanceBuffer != NULL)
			? command->instanceBuffer->buffer
			: geometry->instanceBuffer.buffer;
		for (int i = command->pipeline->firstInstanceBinding;
			i < command->pipeline->vertexBindingCount; i++)
		{
			VC(device->vkCmdBindVertexBuffers(cmdBuffer, i, 1, &instanceBuffer,
				&command->pipeline->vertexBindingOffsets[i]));
		}

		const VkIndexType indexType = (sizeof(GpuTriangleIndex) == sizeof(unsigned int))
			? VK_INDEX_TYPE_UINT32
			: VK_INDEX_TYPE_UINT16;
		VC(device->vkCmdBindIndexBuffer(cmdBuffer, geometry->indexBuffer.buffer, 0, indexType));
	}

	VC(device->vkCmdDrawIndexed(cmdBuffer, geometry->indexCount, command->numInstances, 0, 0,
		0));

	this->currentGraphicsState = *command;
}
void GpuCommandBuffer::SubmitComputeCommand( const GpuComputeCommand* command ) {
	assert(this->currentRenderPass == NULL);

	GpuDevice* device = this->context->device;

	VkCommandBuffer            cmdBuffer = this->cmdBuffers[this->currentBuffer];
	const GpuComputeCommand* state = &this->currentComputeState;

	// If the pipeline has changed.
	if (command->pipeline != state->pipeline)
	{
		VC(device->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
			command->pipeline->pipeline));
	}

	const GpuProgramParmLayout* commandLayout = &command->pipeline->program->parmLayout;
	const GpuProgramParmLayout* stateLayout =
		(state->pipeline != NULL) ? &state->pipeline->program->parmLayout : NULL;

	GpuCommandBuffer_UpdateProgramParms(commandBuffer, commandLayout, stateLayout,
		&command->parmState, &state->parmState,
		VK_PIPELINE_BIND_POINT_COMPUTE);

	VC(device->vkCmdDispatch(cmdBuffer, command->x, command->y, command->z));

	this->currentComputeState = *command;
}

GpuBuffer* GpuCommandBuffer::MapBuffer( GpuBuffer* buffer, void** data ) {
	assert(this->currentRenderPass == NULL);

	GpuDevice* device = this->context->device;

	GpuBuffer* newBuffer = NULL;
	for (GpuBuffer** b = &this->oldMappedBuffers[this->currentBuffer];
		*b != NULL; b = &(*b)->next)
	{
		if ((*b)->size == buffer->size && (*b)->type == buffer->type)
		{
			newBuffer = *b;
			*b = (*b)->next;
			break;
		}
	}
	if (newBuffer == NULL)
	{
		newBuffer = (GpuBuffer*)malloc(sizeof(GpuBuffer));
		GpuBuffer_Create(this->context, newBuffer, buffer->type, buffer->size, NULL,
			true);
	}

	newBuffer->unusedCount = 0;
	newBuffer->next = this->mappedBuffers[this->currentBuffer];
	this->mappedBuffers[this->currentBuffer] = newBuffer;

	assert(newBuffer->mapped == NULL);
	VK(device->vkMapMemory(this->context->device->device, newBuffer->memory, 0,
		newBuffer->size, 0, &newBuffer->mapped));

	*data = newBuffer->mapped;

	return newBuffer;
}
void       GpuCommandBuffer::UnmapBuffer( GpuBuffer* buffer, GpuBuffer* mappedBuffer,
                                    const GpuBufferUnmapType type )
{
	// Can only copy or issue memory barrier outside a render pass.
	assert(this->currentRenderPass == NULL);

	GpuDevice* device = this->context->device;

	VC(device->vkUnmapMemory(this->context->device->device, mappedBuffer->memory));
	mappedBuffer->mapped = NULL;

	// Optionally copy the mapped buffer back to the original buffer. While the copy is not for free,
	// there may be a performance benefit from using the original buffer if it lives in device local memory.
	if (type == GPU_BUFFER_UNMAP_TYPE_COPY_BACK)
	{
		assert(buffer->size == mappedBuffer->size);

		{
			// Add a memory barrier for the mapped buffer from host write to DMA read.
			VkBufferMemoryBarrier bufferMemoryBarrier;
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.pNext = NULL;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = mappedBuffer->buffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = mappedBuffer->size;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_HOST_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
			const VkDependencyFlags    flags = 0;

			VC(device->vkCmdPipelineBarrier(
				this->cmdBuffers[this->currentBuffer], src_stages, dst_stages,
				flags, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL));
		}

		{
			// Copy back to the original buffer.
			VkBufferCopy bufferCopy;
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = buffer->size;

			VC(device->vkCmdCopyBuffer(this->cmdBuffers[this->currentBuffer],
				mappedBuffer->buffer, buffer->buffer, 1, &bufferCopy));
		}

		{
			// Add a memory barrier for the original buffer from DMA write to the buffer access.
			VkBufferMemoryBarrier bufferMemoryBarrier;
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.pNext = NULL;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = GpuBuffer_GetBufferAccess(buffer->type);
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = buffer->buffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = buffer->size;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
			const VkPipelineStageFlags dst_stages =
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // fixme: optimise
			const VkDependencyFlags flags = 0;

			VC(device->vkCmdPipelineBarrier(
				this->cmdBuffers[this->currentBuffer], src_stages, dst_stages,
				flags, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL));
		}
	}
	else
	{
		{
			// Add a memory barrier for the mapped buffer from host write to buffer access.
			VkBufferMemoryBarrier bufferMemoryBarrier;
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.pNext = NULL;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = GpuBuffer_GetBufferAccess(mappedBuffer->type);
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = mappedBuffer->buffer;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = mappedBuffer->size;

			const VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_HOST_BIT;
			const VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			const VkDependencyFlags    flags = 0;

			VC(device->vkCmdPipelineBarrier(
				this->cmdBuffers[this->currentBuffer], src_stages, dst_stages,
				flags, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL));
		}
	}
}

GpuBuffer* GpuCommandBuffer::MapVertexAttributes( GpuGeometry*              geometry,
                                                  GpuVertexAttributeArrays* attribs )
{
	void*        data = NULL;
	GpuBuffer* buffer =
		GpuCommandBuffer_MapBuffer(commandBuffer, &geometry->vertexBuffer, &data);

	attribs->layout = geometry->layout;
	GpuVertexAttributeArrays_Map(attribs, data, buffer->size, geometry->vertexCount,
		geometry->vertexAttribsFlags);

	return buffer;
}
void GpuCommandBuffer::UnmapVertexAttributes( GpuGeometry* geometry, GpuBuffer* mappedVertexBuffer,
                                              const GpuBufferUnmapType type )
{
	GpuCommandBuffer_UnmapBuffer(commandBuffer, &geometry->vertexBuffer, mappedVertexBuffer,
		type);
}

GpuBuffer* GpuCommandBuffer::MapInstanceAttributes( GpuGeometry*              geometry,
                                                    GpuVertexAttributeArrays* attribs )
{
	void*        data = NULL;
	GpuBuffer* buffer =
		GpuCommandBuffer_MapBuffer(commandBuffer, &geometry->instanceBuffer, &data);

	attribs->layout = geometry->layout;
	GpuVertexAttributeArrays_Map(attribs, data, buffer->size, geometry->instanceCount,
		geometry->instanceAttribsFlags);

	return buffer;
}
void GpuCommandBuffer::UnmapInstanceAttributes( GpuGeometry*             geometry,
                                                GpuBuffer*               mappedInstanceBuffer,
                                                const GpuBufferUnmapType type )
{
	GpuCommandBuffer_UnmapBuffer(commandBuffer, &geometry->instanceBuffer, mappedInstanceBuffer,
		type);
}

} // namespace lxd
