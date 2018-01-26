#include "GpuGraphicsPipeline.hpp"
#include "GpuDevice.hpp"
#include "GpuVertexAttribute.hpp"
#include "GpuGeometry.hpp"
#include "GpuGraphicsProgram.hpp"
#include "GpuRenderPass.hpp"
#include <algorithm>

namespace lxd
{
void InitVertexAttributes( const bool instance, const GpuVertexAttribute* vertexLayout,
                           const int numAttribs, const int storedAttribsFlags,
                           const int                          usedAttribsFlags,
                           VkVertexInputAttributeDescription* attributes, int* attributeCount,
                           VkVertexInputBindingDescription* bindings, int* bindingCount,
                           VkDeviceSize* bindingOffsets )
{
    size_t offset = 0;
    for ( int i = 0; vertexLayout[i].attributeFlag != 0; i++ )
    {
        const GpuVertexAttribute* v = &vertexLayout[i];
        if ( ( v->attributeFlag & storedAttribsFlags ) != 0 )
        {
            if ( ( v->attributeFlag & usedAttribsFlags ) != 0 )
            {
                for ( int location = 0; location < v->locationCount; location++ )
                {
                    attributes[*attributeCount + location].location = *attributeCount + location;
                    attributes[*attributeCount + location].binding  = *bindingCount;
                    attributes[*attributeCount + location].format   = (VkFormat)v->attributeFormat;
                    attributes[*attributeCount + location].offset   = ( uint32_t )(
                        location * v->attributeSize /
                        v->locationCount ); // limited offset used for packed vertex data
                }

                bindings[*bindingCount].binding = *bindingCount;
                bindings[*bindingCount].stride  = (uint32_t)v->attributeSize;
                bindings[*bindingCount].inputRate =
                    instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

                bindingOffsets[*bindingCount] =
                    (VkDeviceSize)offset; // memory offset within vertex buffer

                *attributeCount += v->locationCount;
                *bindingCount += 1;
            }
            offset += numAttribs * v->attributeSize;
        }
    }
}

GpuGraphicsPipeline::GpuGraphicsPipeline( GpuContext*                     context,
                                          const GpuGraphicsPipelineParms* parms )
    : context( *context )
{

    // Make sure the geometry provides all the attributes needed by the program.
    assert( ( ( parms->geometry->vertexAttribsFlags | parms->geometry->instanceAttribsFlags ) &
              parms->program->vertexAttribsFlags ) == parms->program->vertexAttribsFlags );

    this->rop                  = parms->rop;
    this->program              = parms->program;
    this->geometry             = parms->geometry;
    this->vertexAttributeCount = 0;
    this->vertexBindingCount   = 0;

    InitVertexAttributes( false, parms->geometry->layout, parms->geometry->vertexCount,
                          parms->geometry->vertexAttribsFlags, parms->program->vertexAttribsFlags,
                          this->vertexAttributes, &this->vertexAttributeCount, this->vertexBindings,
                          &this->vertexBindingCount, this->vertexBindingOffsets );

    this->firstInstanceBinding = this->vertexBindingCount;

    InitVertexAttributes( true, parms->geometry->layout, parms->geometry->instanceCount,
                          parms->geometry->instanceAttribsFlags, parms->program->vertexAttribsFlags,
                          this->vertexAttributes, &this->vertexAttributeCount, this->vertexBindings,
                          &this->vertexBindingCount, this->vertexBindingOffsets );

    this->vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    this->vertexInputState.pNext = NULL;
    this->vertexInputState.flags = 0;
    this->vertexInputState.vertexBindingDescriptionCount   = this->vertexBindingCount;
    this->vertexInputState.pVertexBindingDescriptions      = this->vertexBindings;
    this->vertexInputState.vertexAttributeDescriptionCount = this->vertexAttributeCount;
    this->vertexInputState.pVertexAttributeDescriptions    = this->vertexAttributes;

    this->inputAssemblyState.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    this->inputAssemblyState.pNext    = NULL;
    this->inputAssemblyState.flags    = 0;
    this->inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    this->inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo;
    tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationStateCreateInfo.pNext = NULL;
    tessellationStateCreateInfo.flags = 0;
    tessellationStateCreateInfo.patchControlPoints = 0;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
    viewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pNext         = NULL;
    viewportStateCreateInfo.flags         = 0;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports    = NULL;
    viewportStateCreateInfo.scissorCount  = 1;
    viewportStateCreateInfo.pScissors     = NULL;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.pNext = NULL;
    rasterizationStateCreateInfo.flags = 0;
    rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode                = (VkCullModeFlags)parms->rop.cullMode;
    rasterizationStateCreateInfo.frontFace               = (VkFrontFace)parms->rop.frontFace;
    rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp          = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor    = 0.0f;
    rasterizationStateCreateInfo.lineWidth               = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.pNext = NULL;
    multisampleStateCreateInfo.flags = 0;
    multisampleStateCreateInfo.rasterizationSamples =
        (VkSampleCountFlagBits)parms->renderPass->sampleCount;
    multisampleStateCreateInfo.sampleShadingEnable   = VK_FALSE;
    multisampleStateCreateInfo.minSampleShading      = 1.0f;
    multisampleStateCreateInfo.pSampleMask           = NULL;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable      = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.pNext = NULL;
    depthStencilStateCreateInfo.flags = 0;
    depthStencilStateCreateInfo.depthTestEnable  = parms->rop.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencilStateCreateInfo.depthWriteEnable = parms->rop.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp   = (VkCompareOp)parms->rop.depthCompare;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable     = VK_FALSE;
    depthStencilStateCreateInfo.front.failOp          = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.front.passOp          = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.front.depthFailOp     = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.front.compareOp       = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCreateInfo.back.failOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.passOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.depthFailOp      = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.compareOp        = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCreateInfo.minDepthBounds        = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds        = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachementState[1];
    colorBlendAttachementState[0].blendEnable         = parms->rop.blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachementState[0].srcColorBlendFactor = (VkBlendFactor)parms->rop.blendSrcColor;
    colorBlendAttachementState[0].dstColorBlendFactor = (VkBlendFactor)parms->rop.blendDstColor;
    colorBlendAttachementState[0].colorBlendOp        = (VkBlendOp)parms->rop.blendOpColor;
    colorBlendAttachementState[0].srcAlphaBlendFactor = (VkBlendFactor)parms->rop.blendSrcAlpha;
    colorBlendAttachementState[0].dstAlphaBlendFactor = (VkBlendFactor)parms->rop.blendDstAlpha;
    colorBlendAttachementState[0].alphaBlendOp        = (VkBlendOp)parms->rop.blendOpAlpha;
    colorBlendAttachementState[0].colorWriteMask =
        ( parms->rop.redWriteEnable ? VK_COLOR_COMPONENT_R_BIT : 0 ) |
        ( parms->rop.blueWriteEnable ? VK_COLOR_COMPONENT_G_BIT : 0 ) |
        ( parms->rop.greenWriteEnable ? VK_COLOR_COMPONENT_B_BIT : 0 ) |
        ( parms->rop.alphaWriteEnable ? VK_COLOR_COMPONENT_A_BIT : 0 );

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.pNext = NULL;
    colorBlendStateCreateInfo.flags = 0;
    colorBlendStateCreateInfo.logicOpEnable     = VK_FALSE;
    colorBlendStateCreateInfo.logicOp           = VK_LOGIC_OP_CLEAR;
    colorBlendStateCreateInfo.attachmentCount   = 1;
    colorBlendStateCreateInfo.pAttachments      = colorBlendAttachementState;
    colorBlendStateCreateInfo.blendConstants[0] = parms->rop.blendColor.x;
    colorBlendStateCreateInfo.blendConstants[1] = parms->rop.blendColor.y;
    colorBlendStateCreateInfo.blendConstants[2] = parms->rop.blendColor.z;
    colorBlendStateCreateInfo.blendConstants[3] = parms->rop.blendColor.w;

    VkDynamicState dynamicStateEnables[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.pNext = NULL;
    pipelineDynamicStateCreateInfo.flags = 0;
    pipelineDynamicStateCreateInfo.dynamicStateCount =
        static_cast<uint32_t>( std::size( dynamicStateEnables ) );
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
    graphicsPipelineCreateInfo.sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.pNext             = NULL;
    graphicsPipelineCreateInfo.flags             = 0;
    graphicsPipelineCreateInfo.stageCount        = 2;
    graphicsPipelineCreateInfo.pStages           = parms->program->pipelineStages;
    graphicsPipelineCreateInfo.pVertexInputState = &this->vertexInputState;
    graphicsPipelineCreateInfo.pInputAssemblyState = &this->inputAssemblyState;
    graphicsPipelineCreateInfo.pTessellationState  = &tessellationStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState   = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState =
        ( parms->renderPass->internalDepthFormat != VK_FORMAT_UNDEFINED )
            ? &depthStencilStateCreateInfo
            : NULL;
    graphicsPipelineCreateInfo.pColorBlendState   = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState      = &pipelineDynamicStateCreateInfo;
    graphicsPipelineCreateInfo.layout             = parms->program->parmLayout.pipelineLayout;
    graphicsPipelineCreateInfo.renderPass         = parms->renderPass->renderPass;
    graphicsPipelineCreateInfo.subpass            = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex  = 0;

    VK( context->device->vkCreateGraphicsPipelines( context->device->device, context->pipelineCache,
                                                    1, &graphicsPipelineCreateInfo, VK_ALLOCATOR,
                                                    &this->pipeline ) );
}

GpuGraphicsPipeline::~GpuGraphicsPipeline()
{
    VC( context.device->vkDestroyPipeline( context.device->device, this->pipeline, VK_ALLOCATOR ) );

    memset( pipeline, 0, sizeof( GpuGraphicsPipeline ) );
}
} // namespace lxd
