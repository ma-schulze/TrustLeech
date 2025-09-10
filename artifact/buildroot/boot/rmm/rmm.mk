

 

RMM_CMAKE_OPTS += \
	 $(call qstrip,$(BR2_TARGET_REALM_MANAGEMENT_MONITOR_ADDITIONAL_VARIABLES)) \
	-DRMM_CONFIG=$(BR2_TARGET_REALM_MANAGEMENT_MONITOR_RMM_CONFIG) \
	-S $(@D) -B $(@D)/build && \
	cmake --build $(@D)/build 


define RMM_BUILD_CMDS
	export CROSS_COMPILE="/arm-gnu-toolchain-12.3.rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf-" && cmake $(RMM_CMAKE_OPTS)
endef


RMM_INSTALL_IMAGES = YES
define RMM_INSTALL_IMAGES_CMDS
		cp $(@D)/build/Release/rmm.img $(BINARIES_DIR)/
endef

$(eval $(generic-package))
