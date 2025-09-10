set(HYPERVISOR               trustleech)

set(DRIVERS_BIN_PATH ${CMAKE_CURRENT_LIST_DIR}/drivers/virtio-drivers/examples/aarch64/target/aarch64-unknown-none/release/libaarch64.a)
set(DRIVERS aarch64)

set(HYPERVISOR_BIN_NAME      ${HYPERVISOR}.bin)
set(HYPERVISOR_BIN_PATH      ${CMAKE_CURRENT_BINARY_DIR}/${HYPERVISOR_BIN_NAME})
set(HYPERVISOR_BIN_TARGET    ${HYPERVISOR_BIN_NAME}.target)

set(CXXFLAGS_WARNING         -Wall -Wextra)
set(CXXFLAGS_OS              -ffreestanding -fno-exceptions -fno-rtti -fno-pic -fno-pie -nodefaultlibs -nostdlib -nostdinc -g)
set(CXXFLAGS_ARCH            -march=armv8.7-a+nofp -mgeneral-regs-only -mabi=lp64)
set(CXXFLAGS_ALL             ${CXXFLAGS_WARNING} ${CXXFLAGS_OS} ${CXXFLAGS_ARCH})

set(LINKER_SCRIPT_NAME       "qemu.ld")
set(LINKER_SCRIPT_IN_PATH    ${CMAKE_CURRENT_LIST_DIR}/${LINKER_SCRIPT_NAME}.S)
set(LINKER_SCRIPT_OUT_PATH   ${CMAKE_CURRENT_BINARY_DIR}/${LINKER_SCRIPT_NAME})
set(LINKER_SCRIPT_OUT_TARGET ${LINKER_SCRIPT_NAME}.target)

set(LDFLAGS                  -static -nostdlib -T${LINKER_SCRIPT_OUT_PATH} ${CXXFLAGS_WARNING} ${CXXFLAGS_OS} -g)

# Add executable
add_executable(${HYPERVISOR}
	arch.hpp
	assert.cpp
	assert.hpp
	bit.hpp
	bitfield.hpp
	context_stash.cpp
	context_stash.hpp
	exception/eret.cpp
	exception/eret.hpp
	exception/head.S
	exception/head.cpp
	exception/head.hpp
	exception/inject.cpp
	exception/inject.hpp
	exception/mmu.cpp
	exception/mmu.hpp
	exception/system.cpp
	exception/system.hpp
	exception/types.hpp
	exec_vel2.cpp
	exec_vel2.hpp
	exec_vm.cpp
	exec_vm.hpp
	glibc-sysdep.h
	head.S
    spinlock.S 
    spinlock.hpp 
	head.cpp
	libc.hpp
	macros.S
	memcpy.cpp
	memory.hpp
	memory_backed_regs.cpp
	memory_backed_regs.hpp
	paging/64k/descriptor.hpp
	paging/64k/input_address.hpp
	paging/64k/region.hpp
	paging/constants.hpp
	paging/frame_allocator.cpp
	paging/frame_allocator.hpp
	paging/shadow/descriptor.hpp
	paging/shadow/shadow.cpp
	paging/shadow/shadow.hpp
	paging/shadow/table_walk.cpp
	paging/shadow/table_walk.hpp
	paging/stage_1.cpp
	paging/stage_1.hpp
	paging/stage_2.hpp
	paging/types.hpp
	paging/vpas.cpp
	paging/vpas.hpp
	plat/feat.hpp
	plat/paging.hpp
	plat/qemu/feat.hpp
	plat/qemu/paging.hpp
	registers.hpp
	sysops.hpp
	tail.S
	tail.cpp
	tail.hpp
	type_traits.hpp
	types.hpp
	utility.hpp
	vcpu_ctx.cpp
	vcpu_ctx.hpp
    lib/libc/errno.hpp
    lib/libc/stdio.hpp
    lib/libc/stdarg.hpp
    lib/libc/stdbool.hpp
    lib/libc/stddef.hpp
    lib/libc/stdint.hpp
    lib/console/console.cpp 
    lib/console/console.hpp 
    lib/console/printf.cpp 
    drivers/mmio.hpp
    drivers/network.cpp 
    drivers/network.hpp
    dbg/dbg.cpp 
    dbg/dbg.hpp
    dbg/vmi.cpp
    dbg/vmi.hpp 
    dbg/vmi_handlers.cpp
    dbg/vmi_mm.cpp
    dbg/vmi_mm.hpp
    dbg/vmi_events.cpp
    dbg/vmi_events.hpp
)



target_compile_options(${HYPERVISOR}
	PRIVATE ${CXXFLAGS_ALL}
)

target_include_directories(${HYPERVISOR}
	PRIVATE ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_features(${HYPERVISOR}
	PRIVATE cxx_std_23
)

target_link_directories(${HYPERVISOR} PUBLIC ${DRIVERS_BIN_PATH})

target_link_libraries(${HYPERVISOR} 
  PUBLIC ${DRIVERS_BIN_PATH}
  PRIVATE gcc arm_trusted_firmware
)

target_link_options(${HYPERVISOR}
	PRIVATE ${LDFLAGS}
)

# Preprocess Linker script
add_custom_command(
	OUTPUT ${LINKER_SCRIPT_OUT_PATH}
	DEPENDS ${LINKER_SCRIPT_IN_PATH}
	COMMAND ${CMAKE_CXX_COMPILER} -E -P -x c++ -I${ARM_TRUSTED_FIRMWARE_INCLUDE_DIRECTORIES} -o ${LINKER_SCRIPT_OUT_PATH} ${LINKER_SCRIPT_IN_PATH}
	COMMENT "Preprocessing linker script '${LINKER_SCRIPT_IN_PATH}'"
)

add_custom_target(${LINKER_SCRIPT_OUT_TARGET}
	DEPENDS
		${LINKER_SCRIPT_OUT_PATH}
)

set_target_properties(${HYPERVISOR}
	PROPERTIES
		LINK_DEPENDS ${LINKER_SCRIPT_OUT_PATH}
)

# Add extra dependency, since LINK_DEPENDS does not cause the linker script
# output to be generated on first run
add_dependencies(${HYPERVISOR}
	${LINKER_SCRIPT_OUT_TARGET}
)

# Unroll ELF to flat binary
add_custom_command(
	OUTPUT ${HYPERVISOR_BIN_PATH}
	COMMAND ${CMAKE_OBJCOPY} -O binary --set-section-flags .bss=alloc,load,contents $<TARGET_FILE:${HYPERVISOR}> ${HYPERVISOR_BIN_PATH}
	DEPENDS ${HYPERVISOR}
	COMMENT "Unrolling executable to flat binary"
)

add_custom_target(${HYPERVISOR_BIN_TARGET}
	ALL
	DEPENDS
		${HYPERVISOR_BIN_PATH}
)

# Install
install(FILES ${HYPERVISOR_BIN_PATH} DESTINATION "./")
