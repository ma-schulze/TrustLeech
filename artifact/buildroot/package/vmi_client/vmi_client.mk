################################################################################
#
# libvmi 
#
################################################################################

VMI_CLIENT_VERSION = 0.12
VMI_CLIENT_SITE = $(call github,zeromq,cppzmq,v$(LIBVMI_VERSION))
VMI_CLIENT_INSTALL_STAGING = YES
VMI_CLIENT_DEPENDENCIES = libvmi
VMI_CLIENT_LICENSE = MIT
VMI_CLIENT_LICENSE_FILES = LICENSE

define VMI_CLIENT_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/vmi_client $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D $(@D)/vmi_client_2 $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D $(@D)/vmi_client_3 $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D $(@D)/vmi_client_4 $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D $(@D)/System.map $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D $(@D)/libvmi.conf $(TARGET_DIR)/usr/bin/
endef


$(eval $(cmake-package))
