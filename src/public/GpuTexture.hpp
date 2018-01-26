#pragma once
#include "Gfx.hpp"
#include "GpuContext.hpp"
namespace lxd
{
// Note that the channel listed first in the name shall occupy the least significant bit.
typedef enum
{
    //
    // 8 bits per component
    //
    GPU_TEXTURE_FORMAT_R8_UNORM   = VK_FORMAT_R8_UNORM,   // 1-component, 8-bit unsigned normalized
    GPU_TEXTURE_FORMAT_R8G8_UNORM = VK_FORMAT_R8G8_UNORM, // 2-component, 8-bit unsigned normalized
    GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM =
        VK_FORMAT_R8G8B8A8_UNORM, // 4-component, 8-bit unsigned normalized

    GPU_TEXTURE_FORMAT_R8_SNORM   = VK_FORMAT_R8_SNORM,   // 1-component, 8-bit signed normalized
    GPU_TEXTURE_FORMAT_R8G8_SNORM = VK_FORMAT_R8G8_SNORM, // 2-component, 8-bit signed normalized
    GPU_TEXTURE_FORMAT_R8G8B8A8_SNORM =
        VK_FORMAT_R8G8B8A8_SNORM, // 4-component, 8-bit signed normalized

    GPU_TEXTURE_FORMAT_R8_UINT   = VK_FORMAT_R8_UINT,   // 1-component, 8-bit unsigned integer
    GPU_TEXTURE_FORMAT_R8G8_UINT = VK_FORMAT_R8G8_UINT, // 2-component, 8-bit unsigned integer
    GPU_TEXTURE_FORMAT_R8G8B8A8_UINT =
        VK_FORMAT_R8G8B8A8_UINT, // 4-component, 8-bit unsigned integer

    GPU_TEXTURE_FORMAT_R8_SINT       = VK_FORMAT_R8_SINT,       // 1-component, 8-bit signed integer
    GPU_TEXTURE_FORMAT_R8G8_SINT     = VK_FORMAT_R8G8_SINT,     // 2-component, 8-bit signed integer
    GPU_TEXTURE_FORMAT_R8G8B8A8_SINT = VK_FORMAT_R8G8B8A8_SINT, // 4-component, 8-bit signed integer

    GPU_TEXTURE_FORMAT_R8_SRGB       = VK_FORMAT_R8_SRGB,       // 1-component, 8-bit sRGB
    GPU_TEXTURE_FORMAT_R8G8_SRGB     = VK_FORMAT_R8G8_SRGB,     // 2-component, 8-bit sRGB
    GPU_TEXTURE_FORMAT_R8G8B8A8_SRGB = VK_FORMAT_R8G8B8A8_SRGB, // 4-component, 8-bit sRGB

    // appended: bgra fromat
    GPU_TEXTURE_FORMAT_B8G8R8A8_UNORM =
        VK_FORMAT_B8G8R8A8_UNORM, // 4-component, 8-bit unsigned normalized
    GPU_TEXTURE_FORMAT_B8G8R8A8_SNORM =
        VK_FORMAT_B8G8R8A8_SNORM, // 4-component, 8-bit signed normalized
    GPU_TEXTURE_FORMAT_B8G8R8A8_UINT =
        VK_FORMAT_B8G8R8A8_UINT, // 4-component, 8-bit unsigned integer
    GPU_TEXTURE_FORMAT_B8G8R8A8_SINT = VK_FORMAT_B8G8R8A8_SINT, // 4-component, 8-bit signed integer

    //
    // 16 bits per component
    //
    GPU_TEXTURE_FORMAT_R16_UNORM = VK_FORMAT_R16_UNORM, // 1-component, 16-bit unsigned normalized
    GPU_TEXTURE_FORMAT_R16G16_UNORM =
        VK_FORMAT_R16G16_UNORM, // 2-component, 16-bit unsigned normalized
    GPU_TEXTURE_FORMAT_R16G16B16A16_UNORM =
        VK_FORMAT_R16G16B16A16_UNORM, // 4-component, 16-bit unsigned normalized

    GPU_TEXTURE_FORMAT_R16_SNORM = VK_FORMAT_R16_SNORM, // 1-component, 16-bit signed normalized
    GPU_TEXTURE_FORMAT_R16G16_SNORM =
        VK_FORMAT_R16G16_SNORM, // 2-component, 16-bit signed normalized
    GPU_TEXTURE_FORMAT_R16G16B16A16_SNORM =
        VK_FORMAT_R16G16B16A16_SNORM, // 4-component, 16-bit signed normalized

    GPU_TEXTURE_FORMAT_R16_UINT    = VK_FORMAT_R16_UINT,    // 1-component, 16-bit unsigned integer
    GPU_TEXTURE_FORMAT_R16G16_UINT = VK_FORMAT_R16G16_UINT, // 2-component, 16-bit unsigned integer
    GPU_TEXTURE_FORMAT_R16G16B16A16_UINT =
        VK_FORMAT_R16G16B16A16_UINT, // 4-component, 16-bit unsigned integer

    GPU_TEXTURE_FORMAT_R16_SINT    = VK_FORMAT_R16_SINT,    // 1-component, 16-bit signed integer
    GPU_TEXTURE_FORMAT_R16G16_SINT = VK_FORMAT_R16G16_SINT, // 2-component, 16-bit signed integer
    GPU_TEXTURE_FORMAT_R16G16B16A16_SINT =
        VK_FORMAT_R16G16B16A16_SINT, // 4-component, 16-bit signed integer

    GPU_TEXTURE_FORMAT_R16_SFLOAT = VK_FORMAT_R16_SFLOAT, // 1-component, 16-bit floating-point
    GPU_TEXTURE_FORMAT_R16G16_SFLOAT =
        VK_FORMAT_R16G16_SFLOAT, // 2-component, 16-bit floating-point
    GPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT =
        VK_FORMAT_R16G16B16A16_SFLOAT, // 4-component, 16-bit floating-point

    //
    // 32 bits per component
    //
    GPU_TEXTURE_FORMAT_R32_UINT    = VK_FORMAT_R32_UINT,    // 1-component, 32-bit unsigned integer
    GPU_TEXTURE_FORMAT_R32G32_UINT = VK_FORMAT_R32G32_UINT, // 2-component, 32-bit unsigned integer
    GPU_TEXTURE_FORMAT_R32G32B32A32_UINT =
        VK_FORMAT_R32G32B32A32_UINT, // 4-component, 32-bit unsigned integer

    GPU_TEXTURE_FORMAT_R32_SINT    = VK_FORMAT_R32_SINT,    // 1-component, 32-bit signed integer
    GPU_TEXTURE_FORMAT_R32G32_SINT = VK_FORMAT_R32G32_SINT, // 2-component, 32-bit signed integer
    GPU_TEXTURE_FORMAT_R32G32B32A32_SINT =
        VK_FORMAT_R32G32B32A32_SINT, // 4-component, 32-bit signed integer

    GPU_TEXTURE_FORMAT_R32_SFLOAT = VK_FORMAT_R32_SFLOAT, // 1-component, 32-bit floating-point
    GPU_TEXTURE_FORMAT_R32G32_SFLOAT =
        VK_FORMAT_R32G32_SFLOAT, // 2-component, 32-bit floating-point
    GPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT =
        VK_FORMAT_R32G32B32A32_SFLOAT, // 4-component, 32-bit floating-point

    //
    // S3TC/DXT/BC
    //
    GPU_TEXTURE_FORMAT_BC1_R8G8B8_UNORM =
        VK_FORMAT_BC1_RGB_UNORM_BLOCK, // 3-component, line through 3D space, unsigned normalized
    GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_UNORM =
        VK_FORMAT_BC1_RGBA_UNORM_BLOCK, // 4-component, line through 3D space plus 1-bit alpha, unsigned normalized
    GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_UNORM =
        VK_FORMAT_BC2_UNORM_BLOCK, // 4-component, line through 3D space plus line through 1D space, unsigned normalized
    GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_UNORM =
        VK_FORMAT_BC3_UNORM_BLOCK, // 4-component, line through 3D space plus 4-bit alpha, unsigned normalized

    GPU_TEXTURE_FORMAT_BC1_R8G8B8_SRGB =
        VK_FORMAT_BC1_RGB_SRGB_BLOCK, // 3-component, line through 3D space, sRGB
    GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_SRGB =
        VK_FORMAT_BC1_RGBA_SRGB_BLOCK, // 4-component, line through 3D space plus 1-bit alpha, sRGB
    GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_SRGB =
        VK_FORMAT_BC2_SRGB_BLOCK, // 4-component, line through 3D space plus line through 1D space, sRGB
    GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_SRGB =
        VK_FORMAT_BC3_SRGB_BLOCK, // 4-component, line through 3D space plus 4-bit alpha, sRGB

    GPU_TEXTURE_FORMAT_BC4_R8_UNORM =
        VK_FORMAT_BC4_UNORM_BLOCK, // 1-component, line through 1D space, unsigned normalized
    GPU_TEXTURE_FORMAT_BC5_R8G8_UNORM =
        VK_FORMAT_BC5_UNORM_BLOCK, // 2-component, two lines through 1D space, unsigned normalized

    GPU_TEXTURE_FORMAT_BC4_R8_SNORM =
        VK_FORMAT_BC4_SNORM_BLOCK, // 1-component, line through 1D space, signed normalized
    GPU_TEXTURE_FORMAT_BC5_R8G8_SNORM =
        VK_FORMAT_BC5_SNORM_BLOCK, // 2-component, two lines through 1D space, signed normalized

    //
    // ETC
    //
    GPU_TEXTURE_FORMAT_ETC2_R8G8B8_UNORM =
        VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, // 3-component ETC2, unsigned normalized
    GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_UNORM =
        VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, // 3-component with 1-bit alpha ETC2, unsigned normalized
    GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_UNORM =
        VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, // 4-component ETC2, unsigned normalized

    GPU_TEXTURE_FORMAT_ETC2_R8G8B8_SRGB =
        VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, // 3-component ETC2, sRGB
    GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_SRGB =
        VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, // 3-component with 1-bit alpha ETC2, sRGB
    GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_SRGB =
        VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, // 4-component ETC2, sRGB

    GPU_TEXTURE_FORMAT_EAC_R11_UNORM =
        VK_FORMAT_EAC_R11_UNORM_BLOCK, // 1-component ETC, line through 1D space, unsigned normalized
    GPU_TEXTURE_FORMAT_EAC_R11G11_UNORM =
        VK_FORMAT_EAC_R11G11_UNORM_BLOCK, // 2-component ETC, two lines through 1D space, unsigned normalized

    GPU_TEXTURE_FORMAT_EAC_R11_SNORM =
        VK_FORMAT_EAC_R11_SNORM_BLOCK, // 1-component ETC, line through 1D space, signed normalized
    GPU_TEXTURE_FORMAT_EAC_R11G11_SNORM =
        VK_FORMAT_EAC_R11G11_SNORM_BLOCK, // 2-component ETC, two lines through 1D space, signed normalized

    //
    // ASTC
    //
    GPU_TEXTURE_FORMAT_ASTC_4x4_UNORM =
        VK_FORMAT_ASTC_4x4_UNORM_BLOCK, // 4-component ASTC, 4x4 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_5x4_UNORM =
        VK_FORMAT_ASTC_5x4_UNORM_BLOCK, // 4-component ASTC, 5x4 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_5x5_UNORM =
        VK_FORMAT_ASTC_5x5_UNORM_BLOCK, // 4-component ASTC, 5x5 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_6x5_UNORM =
        VK_FORMAT_ASTC_6x5_UNORM_BLOCK, // 4-component ASTC, 6x5 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_6x6_UNORM =
        VK_FORMAT_ASTC_6x6_UNORM_BLOCK, // 4-component ASTC, 6x6 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_8x5_UNORM =
        VK_FORMAT_ASTC_8x5_UNORM_BLOCK, // 4-component ASTC, 8x5 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_8x6_UNORM =
        VK_FORMAT_ASTC_8x6_UNORM_BLOCK, // 4-component ASTC, 8x6 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_8x8_UNORM =
        VK_FORMAT_ASTC_8x8_UNORM_BLOCK, // 4-component ASTC, 8x8 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_10x5_UNORM =
        VK_FORMAT_ASTC_10x5_UNORM_BLOCK, // 4-component ASTC, 10x5 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_10x6_UNORM =
        VK_FORMAT_ASTC_10x6_UNORM_BLOCK, // 4-component ASTC, 10x6 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_10x8_UNORM =
        VK_FORMAT_ASTC_10x8_UNORM_BLOCK, // 4-component ASTC, 10x8 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_10x10_UNORM =
        VK_FORMAT_ASTC_10x10_UNORM_BLOCK, // 4-component ASTC, 10x10 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_12x10_UNORM =
        VK_FORMAT_ASTC_12x10_UNORM_BLOCK, // 4-component ASTC, 12x10 blocks, unsigned normalized
    GPU_TEXTURE_FORMAT_ASTC_12x12_UNORM =
        VK_FORMAT_ASTC_12x12_UNORM_BLOCK, // 4-component ASTC, 12x12 blocks, unsigned normalized

    GPU_TEXTURE_FORMAT_ASTC_4x4_SRGB =
        VK_FORMAT_ASTC_4x4_SRGB_BLOCK, // 4-component ASTC, 4x4 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_5x4_SRGB =
        VK_FORMAT_ASTC_5x4_SRGB_BLOCK, // 4-component ASTC, 5x4 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_5x5_SRGB =
        VK_FORMAT_ASTC_5x5_SRGB_BLOCK, // 4-component ASTC, 5x5 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_6x5_SRGB =
        VK_FORMAT_ASTC_6x5_SRGB_BLOCK, // 4-component ASTC, 6x5 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_6x6_SRGB =
        VK_FORMAT_ASTC_6x6_SRGB_BLOCK, // 4-component ASTC, 6x6 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_8x5_SRGB =
        VK_FORMAT_ASTC_8x5_SRGB_BLOCK, // 4-component ASTC, 8x5 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_8x6_SRGB =
        VK_FORMAT_ASTC_8x6_SRGB_BLOCK, // 4-component ASTC, 8x6 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_8x8_SRGB =
        VK_FORMAT_ASTC_8x8_SRGB_BLOCK, // 4-component ASTC, 8x8 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_10x5_SRGB =
        VK_FORMAT_ASTC_10x5_SRGB_BLOCK, // 4-component ASTC, 10x5 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_10x6_SRGB =
        VK_FORMAT_ASTC_10x6_SRGB_BLOCK, // 4-component ASTC, 10x6 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_10x8_SRGB =
        VK_FORMAT_ASTC_10x8_SRGB_BLOCK, // 4-component ASTC, 10x8 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_10x10_SRGB =
        VK_FORMAT_ASTC_10x10_SRGB_BLOCK, // 4-component ASTC, 10x10 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_12x10_SRGB =
        VK_FORMAT_ASTC_12x10_SRGB_BLOCK, // 4-component ASTC, 12x10 blocks, sRGB
    GPU_TEXTURE_FORMAT_ASTC_12x12_SRGB =
        VK_FORMAT_ASTC_12x12_SRGB_BLOCK, // 4-component ASTC, 12x12 blocks, sRGB
} GpuTextureFormat;

typedef enum
{
    GPU_TEXTURE_USAGE_UNDEFINED        = 0b0000,
    GPU_TEXTURE_USAGE_GENERAL          = 0b0001,
    GPU_TEXTURE_USAGE_TRANSFER_SRC     = 0b0010,
    GPU_TEXTURE_USAGE_TRANSFER_DST     = 0b0011,
    GPU_TEXTURE_USAGE_SAMPLED          = 0b0100,
    GPU_TEXTURE_USAGE_STORAGE          = 0b0101,
    GPU_TEXTURE_USAGE_COLOR_ATTACHMENT = 0b0110,
    GPU_TEXTURE_USAGE_PRESENTATION     = 0b0111
} GpuTextureUsage;

typedef unsigned int GpuTextureUsageFlags;

typedef enum
{
    GPU_TEXTURE_WRAP_MODE_REPEAT,
    GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE,
    GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER
} GpuTextureWrapMode;

typedef enum
{
    GPU_TEXTURE_FILTER_NEAREST,
    GPU_TEXTURE_FILTER_LINEAR,
    GPU_TEXTURE_FILTER_BILINEAR
} GpuTextureFilter;

typedef enum
{
    GPU_TEXTURE_DEFAULT_CHECKERBOARD, // 32x32 checkerboard pattern (KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
    GPU_TEXTURE_DEFAULT_PYRAMIDS, // 32x32 block pattern of pyramids (KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
    GPU_TEXTURE_DEFAULT_CIRCLES // 32x32 block pattern with circles (KS_GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
} GpuTextureDefault;

class GpuWindow;
class GpuTexture
{
  public:
    GpuTexture( GpuContext* context );
    ~GpuTexture();

    bool Create2D(const GpuTextureFormat format, const GpuSampleCount sampleCount,
		const int width, const int height, const int mipCount,
		const GpuTextureUsageFlags usageFlags, const void* data,
		const size_t dataSize);
    bool Create2DArray(const GpuTextureFormat format, const GpuSampleCount sampleCount,
		const int width, const int height, const int layerCount,
		const int mipCount, const GpuTextureUsageFlags usageFlags,
		const void* data, const size_t dataSize);
    bool CreateFromSwapchain(const GpuWindow* window, int index);
    bool CreateFromKTX(const char* fileName,
		const unsigned char* buffer, const size_t bufferSize);

	void ChangeUsage(VkCommandBuffer cmdBuffer, const GpuTextureUsage usage);
	void UpdateSampler();
  private:
    bool
    CreateInternal( const char* fileName, const VkFormat format, const GpuSampleCount sampleCount,
                    const int width, const int height, const int depth, const int layerCount,
                    const int faceCount, const int mipCount, const GpuTextureUsageFlags usageFlags,
                    const void* data, const size_t dataSize, const bool mipSizeStored,
                    const VkMemoryPropertyFlagBits memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

  public:
    GpuContext&          context;
	int                  width{};
	int                  height{};
	int                  depth{};
	int                  layerCount{};
	int                  mipCount{};
	GpuSampleCount       sampleCount{};
	GpuTextureUsage      usage{};
	GpuTextureUsageFlags usageFlags{};
	GpuTextureWrapMode   wrapMode{};
	GpuTextureFilter     filter{};
	float                maxAnisotropy{};

	VkFormat       format{};
	VkImageLayout  imageLayout{};
	VkImage        image{};
	VkDeviceMemory memory{};
	VkImageView    view{};
	VkSampler      sampler{};
};
} // namespace lxd
