cmake_minimum_required( VERSION 3.4.1 )

project ( lxd_gfx )

add_library( lxd_gfx STATIC
	public/SysUtils.hpp
	public/nanoseconds.h
	public/threading.h
	public/gl_format.h
	public/vk_format.h
	public/Gfx.hpp
	public/GpuInstance.hpp
	private/GpuInstance.cpp
	public/GpuDevice.hpp
	private/GpuDevice.cpp
	public/GpuContext.hpp
	private/GpuContext.cpp
	public/GpuSwapchain.hpp
	private/GpuSwapchain.cpp
	public/GpuBuffer.hpp
	private/GpuBuffer.cpp
	public/GpuWindow.hpp
	private/GpuWindow.cpp
	public/GpuGraphicsProgram.hpp
	private/GpuGraphicsProgram.cpp
)

set_property(TARGET lxd_gfx PROPERTY CXX_STANDARD 17)
set_property(TARGET lxd_gfx PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories( lxd_gfx PUBLIC ./public )
target_compile_definitions( lxd_gfx PUBLIC OS_WINDOWS=1 _CRT_SECURE_NO_WARNINGS )
target_include_directories( lxd_gfx PUBLIC ./public $ENV{VK_SDK_PATH}/Include )
