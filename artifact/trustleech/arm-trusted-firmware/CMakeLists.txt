add_library(arm_trusted_firmware INTERFACE)

target_include_directories(arm_trusted_firmware INTERFACE include/)

set(ARM_TRUSTED_FIRMWARE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/include" PARENT_SCOPE)
