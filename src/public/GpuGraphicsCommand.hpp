#pragma once
#include "GpuGraphicsProgram.hpp"

namespace lxd
{
class GpuGraphicsPipeline;
class GpuBuffer;
class GpuTexture;
class Vector2;
class Vector3;
class Matrix4x4;
class GpuGraphicsCommand
{
  public:
    GpuGraphicsCommand();
    ~GpuGraphicsCommand();

    void SetPipeline( const GpuGraphicsPipeline* pipeline );
    void SetVertexBuffer( const GpuBuffer* vertexBuffer );
    void SetInstanceBuffer( const GpuBuffer* instanceBuffer );
    void SetParmTextureSampled( const int index, const GpuTexture* texture );
    void SetParmTextureStorage( const int index, const GpuTexture* texture );
    void SetParmBufferUniform( const int index, const GpuBuffer* buffer );
    void SetParmBufferStorage( const int index, const GpuBuffer* buffer );
    void SetParmInt( const int index, const int* value );
    void SetParmFloat( const int index, const float* value );
    void SetParmFloatVector2( const int index, const Vector2* value );
    void SetParmFloatVector3( const int index, const Vector3* value );
    void SetParmFloatVector4( const int index, const Vector3* value );
    void SetParmFloatMatrix4x4( const int index, const Matrix4x4* value );
    void SetNumInstances( const int numInstances );

    const GpuGraphicsPipeline* pipeline = {};
    const GpuBuffer*           vertexBuffer =
        {}; // vertex buffer returned by GpuCommandBuffer_MapVertexAttributes
    const GpuBuffer* instanceBuffer =
        {}; // instance buffer returned by GpuCommandBuffer_MapInstanceAttributes
    GpuProgramParmState parmState    = {};
    int                 numInstances = {};
};
} // namespace lxd
