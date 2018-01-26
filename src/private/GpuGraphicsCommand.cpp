#include "GpuGraphicsCommand.hpp"
#include "GpuGraphicsPipeline.hpp"

namespace lxd
{
GpuGraphicsCommand::GpuGraphicsCommand() {}

GpuGraphicsCommand::~GpuGraphicsCommand() {}

void GpuGraphicsCommand::SetPipeline( const GpuGraphicsPipeline* pipeline )
{
    this->pipeline = pipeline;
}
void GpuGraphicsCommand::SetVertexBuffer( const GpuBuffer* vertexBuffer )
{
    this->vertexBuffer = vertexBuffer;
}
void GpuGraphicsCommand::SetInstanceBuffer( const GpuBuffer* instanceBuffer )
{
    this->instanceBuffer = instanceBuffer;
}
void GpuGraphicsCommand::SetParmTextureSampled( const int index, const GpuTexture* texture )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED, texture );
}
void GpuGraphicsCommand::SetParmTextureStorage( const int index, const GpuTexture* texture )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE, texture );
}
void GpuGraphicsCommand::SetParmBufferUniform( const int index, const GpuBuffer* buffer )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM, buffer );
}
void GpuGraphicsCommand::SetParmBufferStorage( const int index, const GpuBuffer* buffer )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE, buffer );
}
void GpuGraphicsCommand::SetParmInt( const int index, const int* value )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT, value );
}
void GpuGraphicsCommand::SetParmFloat( const int index, const float* value )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT, value );
}
void GpuGraphicsCommand::SetParmFloatVector2( const int index, const Vector2* value )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2, value );
}
void GpuGraphicsCommand::SetParmFloatVector3( const int index, const Vector3* value )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3, value );
}
void GpuGraphicsCommand::SetParmFloatVector4( const int index, const Vector3* value )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4, value );
}
void GpuGraphicsCommand::SetParmFloatMatrix4x4( const int index, const Matrix4x4* value )
{
    this->parmState.SetParm( &this->pipeline->program->parmLayout, index,
                             GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4, value );
}
void GpuGraphicsCommand::SetNumInstances( const int numInstances )
{
    this->numInstances = numInstances;
}
} // namespace lxd
