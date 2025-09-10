################################################################################
#
# libvmi 
#
################################################################################

LIBVMI_VERSION = 0.12
LIBVMI_SITE = $(call github,zeromq,cppzmq,v$(LIBVMI_VERSION))
LIBVMI_INSTALL_STAGING = YES
LIBVMI_DEPENDENCIES = zlib json-c libglib2 libvirt
LIBVMI_LICENSE = MIT
LIBVMI_LICENSE_FILES = LICENSE
LIBVMI_CONF_OPTS =  -DENABLE_XEN=OFF -DENABLE_PAGE_CACHE=OFF -DENABLE_KVM=ON -DENABLE_WINDOWS=OFF -DENABLE_TRUSTLEECH=ON -DENABLE_BAREFLANK=OFF -DENABLE_KVM_LEGACY=ON 
# -DVMI_DEBUG=__VMI_DEBUG_ALL 

$(eval $(cmake-package))
