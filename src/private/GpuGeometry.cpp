#include "GpuGeometry.hpp"

namespace lxd
{
GpuGeometry::GpuGeometry( GpuContext* context, const GpuVertexAttributeArrays* attribs,
                          const GpuTriangleIndexArray* indices, const bool dynamic )
    : vertexBuffer( context, GPU_BUFFER_TYPE_VERTEX, attribs->dataSize, attribs->data, false,
                    dynamic ),
      indexBuffer( context, GPU_BUFFER_TYPE_INDEX,
                   indices->indexCount * sizeof( indices->indexArray[0] ), indices->indexArray,
                   dynamic ),
      instanceBuffer( context, GPU_BUFFER_TYPE_INDEX, 0, nullptr, false, false ) // fixme
{
    this->layout             = attribs->layout;
    this->vertexAttribsFlags = attribs->attribsFlags;
    this->vertexCount        = attribs->vertexCount;
    this->indexCount         = indices->indexCount;
}

GpuGeometry::~GpuGeometry() {}
} // namespace lxd
