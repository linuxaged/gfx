#include "GpuVertexAttribute.hpp"
#include <cstring>
#include <memory>
#include <cassert>
#include "Matrix.hpp"

namespace lxd
{

GpuTriangleIndexArray::GpuTriangleIndexArray( const unsigned int    indexCount,
                                              const unsigned short* data )
{
    this->indexCount = indexCount;
    this->indexArray = (unsigned short*)malloc( indexCount * sizeof( unsigned short ) );
    if ( data != nullptr )
    {
        memcpy( this->indexArray, data, indexCount * sizeof( unsigned short ) );
    }
    this->buffer = nullptr;
}

GpuTriangleIndexArray::~GpuTriangleIndexArray()
{
    free( this->indexArray );
    this->buffer     = nullptr;
    this->indexArray = nullptr;
    this->indexCount = 0;
}

void GpuTriangleIndexArray::CreateFromBuffer( const unsigned int indexCount,
                                              GpuBuffer const*   buffer )
{
    this->buffer     = buffer;
    this->indexCount = indexCount;
}

///
///
///
GpuVertexAttributeArrays::GpuVertexAttributeArrays( const GpuVertexAttribute* layout,
                                                    const unsigned int        vertexCount,
                                                    const int                 attribsFlags )
{
    const size_t dataSize = GetDataSize( layout, vertexCount, attribsFlags );
    void*        data     = malloc( dataSize );
    this->buffer          = nullptr;
    this->layout          = layout;
    this->data            = data;
    this->dataSize        = dataSize;
    this->vertexCount     = vertexCount;
    this->attribsFlags    = attribsFlags;
    Map( data, dataSize, vertexCount, attribsFlags );
}
GpuVertexAttributeArrays::~GpuVertexAttributeArrays()
{
    free( this->data );

    buffer       = nullptr;
    layout       = nullptr;
    data         = nullptr;
    dataSize     = 0;
    vertexCount  = 0;
    attribsFlags = 0;
}
void GpuVertexAttributeArrays::CreateFromBuffer()
{
    this->buffer       = buffer;
    this->layout       = layout;
    this->data         = nullptr;
    this->dataSize     = 0;
    this->vertexCount  = vertexCount;
    this->attribsFlags = attribsFlags;
}
void GpuVertexAttributeArrays::Map( void* data, const size_t dataSize,
                                    const unsigned int vertexCount, const int attribsFlags )
{
    unsigned char* dataBytePtr = (unsigned char*)data;
    size_t         offset      = 0;

    for ( int i = 0; this->layout[i].attributeFlag != 0; i++ )
    {
        const GpuVertexAttribute* v         = &this->layout[i];
        void**                    attribPtr = (void**)( ( (char*)this ) + v->attributeOffset );
        if ( ( v->attributeFlag & attribsFlags ) != 0 )
        {
            *attribPtr = ( dataBytePtr + offset );
            offset += vertexCount * v->attributeSize;
        }
        else
        {
            *attribPtr = nullptr;
        }
    }

    assert( offset == dataSize );
}
size_t GpuVertexAttributeArrays::GetDataSize( const GpuVertexAttribute* layout,
                                              const unsigned int        vertexCount,
                                              const int                 attribsFlags )
{
    size_t totalSize = 0;
    for ( int i = 0; layout[i].attributeFlag != 0; i++ )
    {
        const GpuVertexAttribute* v = &layout[i];
        if ( ( v->attributeFlag & attribsFlags ) != 0 )
        {
            totalSize += v->attributeSize;
        }
    }
    return vertexCount * totalSize;
}

void* GpuVertexAttributeArrays::FindAttribute( char const* name, GpuAttributeFormat const format )
{
    for ( int i = 0; this->layout[i].attributeFlag != 0; i++ )
    {
        const GpuVertexAttribute* v = &this->layout[i];
        if ( v->attributeFormat == format && strcmp( v->name, name ) == 0 )
        {
            void** attribPtr = (void**)( ( (char*)this ) + v->attributeOffset );
            return *attribPtr;
        }
    }
    return nullptr;
}

void GpuVertexAttributeArrays::CalculateTangents( GpuTriangleIndexArray const* indices )
{
    // TODO:
}

///
///
///
typedef enum
{
    VERTEX_ATTRIBUTE_FLAG_POSITION      = 0b0000'0000'0000, // vec3 vertexPosition
    VERTEX_ATTRIBUTE_FLAG_NORMAL        = 0b0000'0000'0010, // vec3 vertexNormal
    VERTEX_ATTRIBUTE_FLAG_TANGENT       = 0b0000'0000'0100, // vec3 vertexTangent
    VERTEX_ATTRIBUTE_FLAG_BINORMAL      = 0b0000'0000'1000, // vec3 vertexBinormal
    VERTEX_ATTRIBUTE_FLAG_COLOR         = 0b0000'0001'0000, // vec4 vertexColor
    VERTEX_ATTRIBUTE_FLAG_UV0           = 0b0000'0010'0000, // vec2 vertexUv0
    VERTEX_ATTRIBUTE_FLAG_UV1           = 0b0000'0100'0000, // vec2 vertexUv1
    VERTEX_ATTRIBUTE_FLAG_UV2           = 0b0000'1000'0000, // vec2 vertexUv2
    VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES = 0b0001'0000'0000, // vec4 jointIndices
    VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS = 0b0010'0000'0000, // vec4 jointWeights
    VERTEX_ATTRIBUTE_FLAG_TRANSFORM =
        0b0100'0000'0000, // mat4 vertexTransform (NOTE this mat4 takes up 4 attribute locations)
    VERTEX_ATTRIBUTE_FLAG_FONT_PARMS = 0b1000'0000'0000 //
} DefaultVertexAttributeFlags;

typedef struct
{
    GpuVertexAttributeArrays base;
    Vector3*                 position;
    Vector3*                 normal;
    Vector3*                 tangent;
    Vector3*                 binormal;
    Vector4*                 color;
    Vector2*                 uv0;
    Vector2*                 uv1;
    Vector2*                 uv2;
    Vector4*                 jointIndices;
    Vector4*                 jointWeights;
    Matrix4x4*               transform;
    Vector4*                 fontparm;
} DefaultVertexAttributeArrays;

static const GpuVertexAttribute DefaultVertexAttributeLayout[] = {
    { VERTEX_ATTRIBUTE_FLAG_POSITION, offsetof( DefaultVertexAttributeArrays, position ),
      sizeof( DefaultVertexAttributeArrays::position[0] ), GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT, 1,
      "vertexPosition" },
    { VERTEX_ATTRIBUTE_FLAG_NORMAL, offsetof( DefaultVertexAttributeArrays, normal ),
      sizeof( DefaultVertexAttributeArrays::normal[0] ), GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT, 1,
      "vertexNormal" },
    { VERTEX_ATTRIBUTE_FLAG_TANGENT, offsetof( DefaultVertexAttributeArrays, tangent ),
      sizeof( DefaultVertexAttributeArrays::tangent[0] ), GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT, 1,
      "vertexTangent" },
    { VERTEX_ATTRIBUTE_FLAG_BINORMAL, offsetof( DefaultVertexAttributeArrays, binormal ),
      sizeof( DefaultVertexAttributeArrays::binormal[0] ), GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT, 1,
      "vertexBinormal" },
    { VERTEX_ATTRIBUTE_FLAG_COLOR, offsetof( DefaultVertexAttributeArrays, color ),
      sizeof( DefaultVertexAttributeArrays::color[0] ), GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT, 1,
      "vertexColor" },
    { VERTEX_ATTRIBUTE_FLAG_UV0, offsetof( DefaultVertexAttributeArrays, uv0 ),
      sizeof( DefaultVertexAttributeArrays::uv0[0] ), GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT, 1,
      "vertexUv0" },
    { VERTEX_ATTRIBUTE_FLAG_UV1, offsetof( DefaultVertexAttributeArrays, uv1 ),
      sizeof( DefaultVertexAttributeArrays::uv1[0] ), GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT, 1,
      "vertexUv1" },
    { VERTEX_ATTRIBUTE_FLAG_UV2, offsetof( DefaultVertexAttributeArrays, uv2 ),
      sizeof( DefaultVertexAttributeArrays::uv2[0] ), GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT, 1,
      "vertexUv2" },
    { VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES, offsetof( DefaultVertexAttributeArrays, jointIndices ),
      sizeof( DefaultVertexAttributeArrays::jointIndices[0] ),
      GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT, 1, "vertexJointIndices" },
    { VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS, offsetof( DefaultVertexAttributeArrays, jointWeights ),
      sizeof( DefaultVertexAttributeArrays::jointWeights[0] ),
      GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT, 1, "vertexJointWeights" },
    { VERTEX_ATTRIBUTE_FLAG_TRANSFORM, offsetof( DefaultVertexAttributeArrays, transform ),
      sizeof( DefaultVertexAttributeArrays::transform[0] ),
      GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT, 4, "vertexTransform" },
    { VERTEX_ATTRIBUTE_FLAG_FONT_PARMS, offsetof( DefaultVertexAttributeArrays, fontparm ),
      sizeof( DefaultVertexAttributeArrays::fontparm[0] ), GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT,
      1, "vertexFontParms" },
    { 0, 0, 0, GPU_ATTRIBUTE_FORMAT_NONE, 0, "" }
};

} // namespace lxd
