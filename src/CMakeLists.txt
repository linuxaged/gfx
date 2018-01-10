cmake_minimum_required( VERSION 3.4.1 )

project ( lxd_gfx )

add_library( lxd_gfx STATIC
	private/GpuDevice.cpp
	private/GpuInstance.cpp
)

set_property(TARGET lxd_gfx PROPERTY CXX_STANDARD 17)
set_property(TARGET lxd_gfx PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories( lxd_gfx PUBLIC ./public )
target_compile_definitions( lxd_gfx PUBLIC OS_WINDOWS=1 )
target_include_directories( lxd_gfx PUBLIC ./public $ENV{VK_SDK_PATH}/Include )
