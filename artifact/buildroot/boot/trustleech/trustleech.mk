################################################################################
#
# TrustLeech
#
################################################################################

TRUSTLEECH_VERSION = 0.1
TRUSTLEECH_INSTALL_IMAGES = YES
TRUSTLEECH_INSTALL_TARGET = NO
TRUSTLEECH_INSTALL_IMAGES_CMDS = cmake --install $(@D) --prefix $(BINARIES_DIR)

$(eval $(cmake-package))
