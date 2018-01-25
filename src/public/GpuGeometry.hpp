#pragma once

#include "GpuBuffer.hpp"
#include "GpuVertexAttribute.hpp"

namespace lxd
{
class GpuGeometry
{
  public:
    GpuGeometry( GpuContext* context, const GpuVertexAttributeArrays* attribs,
                 const GpuTriangleIndexArray* indices, const bool dynamic = false );
    ~GpuGeometry();

  public:
    const GpuVertexAttribute* layout;
    int                       vertexAttribsFlags;
    int                       instanceAttribsFlags;
    int                       vertexCount;
    int                       instanceCount;
    int                       indexCount;
    GpuBuffer                 vertexBuffer;
    GpuBuffer                 instanceBuffer;
    GpuBuffer                 indexBuffer;
};
} // namespace lxd
