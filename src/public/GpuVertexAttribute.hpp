#pragma once

#include "Gfx.hpp"

namespace lxd
{
class GpuBuffer;

///
///
///
class GpuTriangleIndexArray
{
  public:
    GpuTriangleIndexArray( const unsigned int indexCount, const unsigned short* data );
    ~GpuTriangleIndexArray();
    void CreateFromBuffer( const unsigned int indexCount, GpuBuffer const* buffer );

  public:
    const GpuBuffer* buffer;
    unsigned short*  indexArray;
    int              indexCount;
};

///
///
///
typedef enum
{
    GPU_ATTRIBUTE_FORMAT_NONE                = 0,
    GPU_ATTRIBUTE_FORMAT_R32_SFLOAT          = VK_FORMAT_R32_SFLOAT,
    GPU_ATTRIBUTE_FORMAT_R32G32_SFLOAT       = VK_FORMAT_R32G32_SFLOAT,
    GPU_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT    = VK_FORMAT_R32G32B32_SFLOAT,
    GPU_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT = VK_FORMAT_R32G32B32A32_SFLOAT
} GpuAttributeFormat;

struct GpuVertexAttribute
{
    int    attributeFlag;   // VERTEX_ATTRIBUTE_FLAG_
    size_t attributeOffset; // Offset in bytes to the pointer in ksGpuVertexAttributeArrays
    size_t attributeSize;   // Size in bytes of a single attribute
    GpuAttributeFormat attributeFormat; // Format of the attribute
    int                locationCount;   // Number of attribute locations
    const char*        name;            // Name in vertex program
};

struct GpuVertexAttributeArrays
{
  public:
    GpuVertexAttributeArrays( const GpuVertexAttribute* layout, const unsigned int vertexCount,
                              const int attribsFlags );
    ~GpuVertexAttributeArrays();
    void   CreateFromBuffer();
    void   Map( void* data, const size_t dataSize, const unsigned int vertexCount,
                const int attribsFlags );
    size_t GetDataSize( const GpuVertexAttribute* layout, const unsigned int vertexCount,
                        const int attribsFlags );
    void*  FindAttribute( char const* name, const GpuAttributeFormat format );
    void   CalculateTangents( GpuTriangleIndexArray const* indices );

  public:
    const GpuBuffer*          buffer;
    const GpuVertexAttribute* layout;
    void*                     data{ nullptr };
    size_t                    dataSize{ 0 };
    int                       vertexCount;
    int                       attribsFlags;
};

} // namespace lxd
