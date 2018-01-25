#include "GpuGraphicsProgram.hpp"
#include "GpuContext.hpp"
#include "GpuDevice.hpp"
#include <iterator>
namespace lxd
{
VkShaderStageFlags GpuProgramParm::GetShaderStageFlags( const GpuProgramStageFlags stageFlags )
{
    return ( ( stageFlags & GPU_PROGRAM_STAGE_FLAG_VERTEX ) ? VK_SHADER_STAGE_VERTEX_BIT : 0 ) |
           ( ( stageFlags & GPU_PROGRAM_STAGE_FLAG_FRAGMENT ) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0 ) |
           ( ( stageFlags & GPU_PROGRAM_STAGE_FLAG_COMPUTE ) ? VK_SHADER_STAGE_COMPUTE_BIT : 0 );
}

VkDescriptorType GpuProgramParm::GetDescriptorType( const GpuProgramParmType type )
{
    return ( ( type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
                 ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                 : ( ( type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
                         ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                         : ( ( type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
                                 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                 : ( ( type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
                                         ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                         : VK_DESCRIPTOR_TYPE_MAX_ENUM ) ) ) );
}
bool GpuProgramParm::IsOpaqueBinding( const GpuProgramParmType type )
{
    bool result;
    switch ( type )
    {
        case GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED:
        case GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE:
        case GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM:
        case GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE:
            result = true;
            break;
        default:
            result = false;
    }
    return result;
}
int GpuProgramParm::GetPushConstantSize( const GpuProgramParmType type )
{
    static const int parmSize[GPU_PROGRAM_PARM_TYPE_MAX] = { 0u,
                                                             0u,
                                                             0u,
                                                             0u,
                                                             sizeof( int ),
                                                             sizeof( int[2] ),
                                                             sizeof( int[3] ),
                                                             sizeof( int[4] ),
                                                             sizeof( float ),
                                                             sizeof( float[2] ),
                                                             sizeof( float[3] ),
                                                             sizeof( float[4] ),
                                                             sizeof( float[2][2] ),
                                                             sizeof( float[2][3] ),
                                                             sizeof( float[2][4] ),
                                                             sizeof( float[3][2] ),
                                                             sizeof( float[3][3] ),
                                                             sizeof( float[3][4] ),
                                                             sizeof( float[4][2] ),
                                                             sizeof( float[4][3] ),
                                                             sizeof( float[4][4] ) };
    assert( std::size( parmSize ) == GPU_PROGRAM_PARM_TYPE_MAX );
    return parmSize[type];
}
const char* GpuProgramParm::GetPushConstantGlslType( const GpuProgramParmType type )
{
    static const char* glslType[GPU_PROGRAM_PARM_TYPE_MAX] = {
        "",       "",       "",     "",       "int",    "ivec2",  "ivec3",
        "ivec4",  "float",  "vec2", "vec3",   "vec4",   "mat2",   "mat2x3",
        "mat2x4", "mat3x2", "mat3", "mat3x4", "mat4x2", "mat4x3", "mat4"
    };
    assert( std::size( glslType ) == GPU_PROGRAM_PARM_TYPE_MAX );
    return glslType[type];
}

GpuProgramParmLayout::GpuProgramParmLayout( GpuContext* context, GpuProgramParm const* parms,
                                            const int numParms )
    : context( *context )
{
    this->numParms = numParms;
    this->parms    = parms;

    int numSampledTextureBindings[GPU_PROGRAM_STAGE_MAX] = { 0 };
    int numStorageTextureBindings[GPU_PROGRAM_STAGE_MAX] = { 0 };
    int numUniformBufferBindings[GPU_PROGRAM_STAGE_MAX]  = { 0 };
    int numStorageBufferBindings[GPU_PROGRAM_STAGE_MAX]  = { 0 };

    int offset = 0;
    memset( this->offsetForIndex, -1, sizeof( this->offsetForIndex ) );

    for ( int i = 0; i < numParms; i++ )
    {
        if ( parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED ||
             parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE ||
             parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM ||
             parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
        {
            for ( int stageIndex = 0; stageIndex < GPU_PROGRAM_STAGE_MAX; stageIndex++ )
            {
                if ( ( parms[i].stageFlags & ( 1 << stageIndex ) ) != 0 )
                {
                    numSampledTextureBindings[stageIndex] +=
                        static_cast<int>( parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED );
                    numStorageTextureBindings[stageIndex] +=
						static_cast<int>( parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE );
                    numUniformBufferBindings[stageIndex] +=
						static_cast<int>( parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM );
                    numStorageBufferBindings[stageIndex] +=
						static_cast<int>( parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE );
                }
            }

            assert( parms[i].binding >= 0 && parms[i].binding < MAX_PROGRAM_PARMS );

            // Make sure each binding location is only used once.
            assert( this->bindings[parms[i].binding] == nullptr );

            this->bindings[parms[i].binding] = &parms[i];
            if ( parms[i].binding >= this->numBindings )
            {
                this->numBindings = parms[i].binding + 1;
            }
        }
        else
        {
            assert( this->numPushConstants < MAX_PROGRAM_PARMS );
            this->pushConstants[this->numPushConstants++] = &parms[i];

            this->offsetForIndex[parms[i].index] = offset;
            offset += GpuProgramParm::GetPushConstantSize( parms[i].type );
        }
    }

    // Make sure the descriptor bindings are packed.
    for ( int binding = 0; binding < this->numBindings; binding++ )
    {
        assert( this->bindings[binding] != nullptr );
    }

    // Verify the push constant layout.
    for ( int push0 = 0; push0 < this->numPushConstants; push0++ )
    {
        // The push constants for a pipeline cannot use more than 'maxPushConstantsSize' bytes.
        assert( this->pushConstants[push0]->binding +
                    GpuProgramParm::GetPushConstantSize( this->pushConstants[push0]->type ) <=
                (int)context->device->physicalDeviceProperties.limits.maxPushConstantsSize );
        // Make sure no push constants overlap.
        for ( int push1 = push0 + 1; push1 < this->numPushConstants; push1++ )
        {
            assert(
                this->pushConstants[push0]->binding >=
                    this->pushConstants[push1]->binding +
                        GpuProgramParm::GetPushConstantSize( this->pushConstants[push1]->type ) ||
                this->pushConstants[push0]->binding +
                        GpuProgramParm::GetPushConstantSize( this->pushConstants[push0]->type ) <=
                    this->pushConstants[push1]->binding );
        }
    }

    // Check the descriptor limits.
    int numTotalSampledTextureBindings = 0;
    int numTotalStorageTextureBindings = 0;
    int numTotalUniformBufferBindings  = 0;
    int numTotalStorageBufferBindings  = 0;
    for ( int stage = 0; stage < GPU_PROGRAM_STAGE_MAX; stage++ )
    {
        assert( numSampledTextureBindings[stage] <=
                (int)context->device->physicalDeviceProperties.limits
                    .maxPerStageDescriptorSampledImages );
        assert( numStorageTextureBindings[stage] <=
                (int)context->device->physicalDeviceProperties.limits
                    .maxPerStageDescriptorStorageImages );
        assert( numUniformBufferBindings[stage] <=
                (int)context->device->physicalDeviceProperties.limits
                    .maxPerStageDescriptorUniformBuffers );
        assert( numStorageBufferBindings[stage] <=
                (int)context->device->physicalDeviceProperties.limits
                    .maxPerStageDescriptorStorageBuffers );

        numTotalSampledTextureBindings += numSampledTextureBindings[stage];
        numTotalStorageTextureBindings += numStorageTextureBindings[stage];
        numTotalUniformBufferBindings += numUniformBufferBindings[stage];
        numTotalStorageBufferBindings += numStorageBufferBindings[stage];
    }

    assert( numTotalSampledTextureBindings <=
            (int)context->device->physicalDeviceProperties.limits.maxDescriptorSetSampledImages );
    assert( numTotalStorageTextureBindings <=
            (int)context->device->physicalDeviceProperties.limits.maxDescriptorSetStorageImages );
    assert( numTotalUniformBufferBindings <=
            (int)context->device->physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers );
    assert( numTotalStorageBufferBindings <=
            (int)context->device->physicalDeviceProperties.limits.maxDescriptorSetStorageBuffers );

    //
    // Create descriptor set layout and pipeline layout
    //

    {
        VkDescriptorSetLayoutBinding descriptorSetBindings[MAX_PROGRAM_PARMS];
        VkPushConstantRange          pushConstantRanges[MAX_PROGRAM_PARMS];

        int numDescriptorSetBindings = 0;
        int numPushConstantRanges    = 0;
        for ( int i = 0; i < numParms; i++ )
        {
            if ( GpuProgramParm::IsOpaqueBinding( parms[i].type ) )
            {
                descriptorSetBindings[numDescriptorSetBindings].binding = parms[i].binding;
                descriptorSetBindings[numDescriptorSetBindings].descriptorType =
                    GpuProgramParm::GetDescriptorType( parms[i].type );
                descriptorSetBindings[numDescriptorSetBindings].descriptorCount = 1;
                descriptorSetBindings[numDescriptorSetBindings].stageFlags =
                    GpuProgramParm::GetShaderStageFlags(
                        static_cast<GpuProgramStageFlags>( parms[i].stageFlags ) );
                // fixme: error on iOS
                //                assert( descriptorSetBindings[numDescriptorSetBindings].stageFlags ==
                //                            VK_SHADER_STAGE_VERTEX_BIT ||
                //                        descriptorSetBindings[numDescriptorSetBindings].stageFlags ==
                //                            VK_SHADER_STAGE_FRAGMENT_BIT );
                descriptorSetBindings[numDescriptorSetBindings].pImmutableSamplers = nullptr;
                numDescriptorSetBindings++;
            }
            else // push constant
            {
                pushConstantRanges[numPushConstantRanges].stageFlags =
                    GpuProgramParm::GetShaderStageFlags(
                        static_cast<GpuProgramStageFlags>( parms[i].stageFlags ) );
                pushConstantRanges[numPushConstantRanges].offset = parms[i].binding;
                pushConstantRanges[numPushConstantRanges].size =
                    GpuProgramParm::GetPushConstantSize( parms[i].type );
                numPushConstantRanges++;
            }
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pNext = nullptr;
        descriptorSetLayoutCreateInfo.flags = 0;
        descriptorSetLayoutCreateInfo.bindingCount = numDescriptorSetBindings;
        descriptorSetLayoutCreateInfo.pBindings =
            ( numDescriptorSetBindings != 0 ) ? descriptorSetBindings : nullptr;

        VK( context->device->vkCreateDescriptorSetLayout(
            context->device->device, &descriptorSetLayoutCreateInfo, VK_ALLOCATOR,
            &this->descriptorSetLayout ) );

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext          = nullptr;
        pipelineLayoutCreateInfo.flags          = 0;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts    = &this->descriptorSetLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = numPushConstantRanges;
        pipelineLayoutCreateInfo.pPushConstantRanges =
            ( numPushConstantRanges != 0 ) ? pushConstantRanges : nullptr;

        VK( context->device->vkCreatePipelineLayout( context->device->device,
                                                     &pipelineLayoutCreateInfo, VK_ALLOCATOR,
                                                     &this->pipelineLayout ) );
    }

    // Calculate a hash of the layout.
    unsigned int hash = 5381;
    for ( int i = 0; i < numParms * (int)sizeof( parms[0] ); i++ )
    {
        hash = ( ( hash << 5 ) - hash ) + ( (const char*)parms )[i];
    }
    this->hash = hash;
}

GpuProgramParmLayout::~GpuProgramParmLayout()
{
    VC( context.device->vkDestroyPipelineLayout( context.device->device, this->pipelineLayout,
                                                 VK_ALLOCATOR ) );
    VC( context.device->vkDestroyDescriptorSetLayout( context.device->device,
                                                      this->descriptorSetLayout, VK_ALLOCATOR ) );
}

GpuGraphicsProgram::GpuGraphicsProgram(
    GpuContext* context, const void* vertexSourceData, const size_t vertexSourceSize,
    const void* fragmentSourceData, const size_t fragmentSourceSize, const GpuProgramParm* parms,
    const int numParms, const GpuVertexAttribute* vertexLayout, const int vertexAttribsFlags )
    : context( *context ), parmLayout( context, parms, numParms )
{
    this->vertexAttribsFlags = vertexAttribsFlags;

    context->device->CreateShader( &this->vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT,
                                   vertexSourceData, vertexSourceSize );
    context->device->CreateShader( &this->fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT,
                                   fragmentSourceData, fragmentSourceSize );

    this->pipelineStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    this->pipelineStages[0].pNext  = nullptr;
    this->pipelineStages[0].flags  = 0;
    this->pipelineStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    this->pipelineStages[0].module = this->vertexShaderModule;
    this->pipelineStages[0].pName  = "main";
    this->pipelineStages[0].pSpecializationInfo = nullptr;

    this->pipelineStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    this->pipelineStages[1].pNext  = nullptr;
    this->pipelineStages[1].flags  = 0;
    this->pipelineStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    this->pipelineStages[1].module = this->fragmentShaderModule;
    this->pipelineStages[1].pName  = "main";
    this->pipelineStages[1].pSpecializationInfo = nullptr;
}

GpuGraphicsProgram::~GpuGraphicsProgram()
{
    VC( context.device->vkDestroyShaderModule( context.device->device, this->vertexShaderModule,
                                               VK_ALLOCATOR ) );
    VC( context.device->vkDestroyShaderModule( context.device->device, this->fragmentShaderModule,
                                               VK_ALLOCATOR ) );
}

} // namespace lxd
