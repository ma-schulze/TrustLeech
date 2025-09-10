cmake_minimum_required(VERSION 3.22.1)

project(trustleech CXX ASM)

# Hack around Buildroot's CMake toolchain file
#
# The Toolchain file is generated based on BR2_ENABLE_DEBUG, but we can not set
# that, since we only debug TrustLeech, not the entire setup. However, after
# Buildroot generates the toolchain file, specific flags are set to fixed
# values, irrespective of the value of CMAKE_BUILD_TYPE...so the
# CMAKE_CXX_FLAGS always include -g0, which is not what we want.
#
# This here has to be done after the project() call, because the toolchain file
# is loaded at that point.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Og -g3 -fsanitize=undefined")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og -g3 -fsanitize=undefined")
endif()

add_subdirectory(arm-trusted-firmware)
add_subdirectory(src)
