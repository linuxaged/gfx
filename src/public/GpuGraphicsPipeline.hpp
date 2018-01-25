#pragma once

#include "Gfx.hpp"
#include "Matrix.hpp"

namespace lxd
{

class GpuRenderPass;
class GpuGraphicsProgram;
class GpuGeometry;

typedef enum
{
    GPU_FRONT_FACE_COUNTER_CLOCKWISE = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    GPU_FRONT_FACE_CLOCKWISE         = VK_FRONT_FACE_CLOCKWISE
} GpuFrontFace;

typedef enum
{
    GPU_CULL_MODE_NONE  = 0,
    GPU_CULL_MODE_FRONT = VK_CULL_MODE_FRONT_BIT,
    GPU_CULL_MODE_BACK  = VK_CULL_MODE_BACK_BIT
} GpuCullMode;

typedef enum
{
    GPU_COMPARE_OP_NEVER            = VK_COMPARE_OP_NEVER,
    GPU_COMPARE_OP_LESS             = VK_COMPARE_OP_LESS,
    GPU_COMPARE_OP_EQUAL            = VK_COMPARE_OP_EQUAL,
    GPU_COMPARE_OP_LESS_OR_EQUAL    = VK_COMPARE_OP_LESS_OR_EQUAL,
    GPU_COMPARE_OP_GREATER          = VK_COMPARE_OP_GREATER,
    GPU_COMPARE_OP_NOT_EQUAL        = VK_COMPARE_OP_NOT_EQUAL,
    GPU_COMPARE_OP_GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL,
    GPU_COMPARE_OP_ALWAYS           = VK_COMPARE_OP_ALWAYS
} GpuCompareOp;

typedef enum
{
    GPU_BLEND_OP_ADD              = VK_BLEND_OP_ADD,
    GPU_BLEND_OP_SUBTRACT         = VK_BLEND_OP_SUBTRACT,
    GPU_BLEND_OP_REVERSE_SUBTRACT = VK_BLEND_OP_REVERSE_SUBTRACT,
    GPU_BLEND_OP_MIN              = VK_BLEND_OP_MIN,
    GPU_BLEND_OP_MAX              = VK_BLEND_OP_MAX
} GpuBlendOp;

typedef enum
{
    GPU_BLEND_FACTOR_ZERO                     = VK_BLEND_FACTOR_ZERO,
    GPU_BLEND_FACTOR_ONE                      = VK_BLEND_FACTOR_ONE,
    GPU_BLEND_FACTOR_SRC_COLOR                = VK_BLEND_FACTOR_SRC_COLOR,
    GPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR      = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    GPU_BLEND_FACTOR_DST_COLOR                = VK_BLEND_FACTOR_DST_COLOR,
    GPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR      = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    GPU_BLEND_FACTOR_SRC_ALPHA                = VK_BLEND_FACTOR_SRC_ALPHA,
    GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA      = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    GPU_BLEND_FACTOR_DST_ALPHA                = VK_BLEND_FACTOR_DST_ALPHA,
    GPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA      = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    GPU_BLEND_FACTOR_CONSTANT_COLOR           = VK_BLEND_FACTOR_CONSTANT_COLOR,
    GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    GPU_BLEND_FACTOR_CONSTANT_ALPHA           = VK_BLEND_FACTOR_CONSTANT_ALPHA,
    GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
    GPU_BLEND_FACTOR_SRC_ALPHA_SATURATE       = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE
} GpuBlendFactor;

struct GpuRasterOperations
{
    bool           blendEnable;
    bool           redWriteEnable;
    bool           blueWriteEnable;
    bool           greenWriteEnable;
    bool           alphaWriteEnable;
    bool           depthTestEnable;
    bool           depthWriteEnable;
    GpuFrontFace   frontFace;
    GpuCullMode    cullMode;
    GpuCompareOp   depthCompare;
    Vector4        blendColor;
    GpuBlendOp     blendOpColor;
    GpuBlendFactor blendSrcColor;
    GpuBlendFactor blendDstColor;
    GpuBlendOp     blendOpAlpha;
    GpuBlendFactor blendSrcAlpha;
    GpuBlendFactor blendDstAlpha;
};

struct ksGpuGraphicsPipelineParms
{
    ksGpuGraphicsPipelineParms()
    {
        this->rop.blendEnable      = false;
        this->rop.redWriteEnable   = true;
        this->rop.blueWriteEnable  = true;
        this->rop.greenWriteEnable = true;
        this->rop.alphaWriteEnable = false;
        this->rop.depthTestEnable  = true;
        this->rop.depthWriteEnable = true;
        this->rop.frontFace        = GPU_FRONT_FACE_COUNTER_CLOCKWISE;
        this->rop.cullMode         = GPU_CULL_MODE_BACK;
        this->rop.depthCompare     = GPU_COMPARE_OP_LESS_OR_EQUAL;
        this->rop.blendColor.x     = 0.0f;
        this->rop.blendColor.y     = 0.0f;
        this->rop.blendColor.z     = 0.0f;
        this->rop.blendColor.w     = 0.0f;
        this->rop.blendOpColor     = GPU_BLEND_OP_ADD;
        this->rop.blendSrcColor    = GPU_BLEND_FACTOR_ONE;
        this->rop.blendDstColor    = GPU_BLEND_FACTOR_ZERO;
        this->rop.blendOpAlpha     = GPU_BLEND_OP_ADD;
        this->rop.blendSrcAlpha    = GPU_BLEND_FACTOR_ONE;
        this->rop.blendDstAlpha    = GPU_BLEND_FACTOR_ZERO;
        this->renderPass           = nullptr;
        this->program              = nullptr;
        this->geometry             = nullptr;
    }
    ~ksGpuGraphicsPipelineParms();

    GpuRasterOperations       rop;
    const GpuRenderPass*      renderPass;
    const GpuGraphicsProgram* program;
    const GpuGeometry*        geometry;
};

class GpuGraphicsPipeline
{
  public:
    GpuGraphicsPipeline();
    ~GpuGraphicsPipeline();

    GpuRasterOperations       rop;
    const GpuGraphicsProgram* program;
    const GpuGeometry*        geometry;

    int                                    vertexAttributeCount;
    int                                    vertexBindingCount;
    int                                    firstInstanceBinding;
    VkVertexInputAttributeDescription      vertexAttributes[MAX_VERTEX_ATTRIBUTES];
    VkVertexInputBindingDescription        vertexBindings[MAX_VERTEX_ATTRIBUTES];
    VkDeviceSize                           vertexBindingOffsets[MAX_VERTEX_ATTRIBUTES];
    VkPipelineVertexInputStateCreateInfo   vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipeline                             pipeline;
};
} // namespace lxd
