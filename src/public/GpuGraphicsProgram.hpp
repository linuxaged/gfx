#pragma once

#include "Gfx.hpp"

namespace lxd
{
enum GpuProgramStageFlags
{
    GPU_PROGRAM_STAGE_FLAG_VERTEX   = 0b0,
    GPU_PROGRAM_STAGE_FLAG_FRAGMENT = 0b1,
    GPU_PROGRAM_STAGE_FLAG_COMPUTE  = 0b10,
    GPU_PROGRAM_STAGE_MAX           = 0b11
};

enum GpuProgramParmType
{
    GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED, // texture plus sampler bound together		(GLSL: sampler*, isampler*, usampler*)
    GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE, // not sampled, direct read-write storage	(GLSL: image*, iimage*, uimage*)
    GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,            // read-only uniform buffer					(GLSL: uniform)
    GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,            // read-write storage buffer				(GLSL: buffer)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,         // int										(GLSL: int)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2, // int[2]									(GLSL: ivec2)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3, // int[3]									(GLSL: ivec3)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4, // int[4]									(GLSL: ivec4)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT,       // float									(GLSL: float)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,   // float[2]									(GLSL: vec2)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3,   // float[3]									(GLSL: vec3)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4,   // float[4]									(GLSL: vec4)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2, // float[2][2]								(GLSL: mat2x2 or mat2)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3, // float[2][3]								(GLSL: mat2x3)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4, // float[2][4]								(GLSL: mat2x4)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2, // float[3][2]								(GLSL: mat3x2)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3, // float[3][3]								(GLSL: mat3x3 or mat3)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4, // float[3][4]								(GLSL: mat3x4)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2, // float[4][2]								(GLSL: mat4x2)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3, // float[4][3]								(GLSL: mat4x3)
    GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4, // float[4][4]								(GLSL: mat4x4 or mat4)
    GPU_PROGRAM_PARM_TYPE_MAX
};

enum GpuProgramParmAccess
{
    GPU_PROGRAM_PARM_ACCESS_READ_ONLY,
    GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,
    GPU_PROGRAM_PARM_ACCESS_READ_WRITE
};

struct GpuProgramParm
{
    int                  stageFlags; // vertex, fragment and/or compute
    GpuProgramParmType   type;       // texture, buffer or push constant
    GpuProgramParmAccess access;     // read and/or write
    int                  index;      // index into ksGpuProgramParmState::parms
    const char*          name;       // GLSL name
    int                  binding;    // OpenGL shader bind points:
                                     // - texture image unit
                                     // - image unit
                                     // - uniform buffer
                                     // - storage buffer
                                     // - uniform
    // Note that each bind point uses its own range of binding indices with each range starting at zero.
    // However, each range is unique across all stages of a pipeline.
    // Note that even though multiple targets can be bound to the same texture image unit,
    // the OpenGL spec disallows rendering from multiple targets using a single texture image unit.

    static VkShaderStageFlags GetShaderStageFlags( const GpuProgramStageFlags stageFlags );
    static VkDescriptorType   GetDescriptorType( const GpuProgramParmType type );
    static bool               IsOpaqueBinding( const GpuProgramParmType type );
    static int                GetPushConstantSize( const GpuProgramParmType type );
    static const char*        GetPushConstantGlslType( const GpuProgramParmType type );
};

class GpuContext;
class GpuProgramParmLayout
{
    static int const MAX_PROGRAM_PARMS = 16;

  public:
    GpuProgramParmLayout( GpuContext* context, GpuProgramParm const* parms, const int numParms );
    ~GpuProgramParmLayout();

  public:
    GpuContext&           context;
    int                   numParms            = 0;
    const GpuProgramParm* parms               = 0;
    VkDescriptorSetLayout descriptorSetLayout = nullptr;
    VkPipelineLayout      pipelineLayout      = nullptr;
    int                   offsetForIndex[MAX_PROGRAM_PARMS] =
        {}; // push constant offsets into ksGpuProgramParmState::data based on ksGpuProgramParm::index
    const GpuProgramParm* bindings[MAX_PROGRAM_PARMS]      = {}; // descriptor bindings
    const GpuProgramParm* pushConstants[MAX_PROGRAM_PARMS] = {}; // push constants
    int                   numBindings                      = 0;
    int                   numPushConstants                 = 0;
    unsigned int          hash                             = 0;
};

class GpuVertexAttribute;
class GpuGraphicsProgram
{
  public:
    GpuGraphicsProgram( GpuContext* context, const void* vertexSourceData,
                        const size_t vertexSourceSize, const void* fragmentSourceData,
                        const size_t fragmentSourceSize, const GpuProgramParm* parms,
                        const int numParms, const GpuVertexAttribute* vertexLayout,
                        const int vertexAttribsFlags );
    ~GpuGraphicsProgram();

  public:
    GpuContext&                     context;
    VkShaderModule                  vertexShaderModule;
    VkShaderModule                  fragmentShaderModule;
    VkPipelineShaderStageCreateInfo pipelineStages[2];
    GpuProgramParmLayout            parmLayout;
    int                             vertexAttribsFlags;
};
} // namespace lxd
