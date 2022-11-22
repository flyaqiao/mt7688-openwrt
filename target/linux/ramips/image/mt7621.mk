#
# MT7621 Profiles
#
include ./common-tp-link.mk

DEFAULT_SOC := mt7621

KERNEL_DTB += -d21
DEVICE_VARS += ELECOM_HWNAME LINKSYS_HWNAME

# define Device/WMD_7621Ultra_12816
#   $(Device/uimage-lzma-loader)
#   IMAGE_SIZE := 16064k
#   DEVICE_VARIANT := 16M
#   DEVICE_MODEL := WMD-7621Ultra-12816
#   DEVICE_PACKAGES := kmod-mt7603
#   SUPPORTED_DEVICES += Wooya,wms-7621a
# endef
# TARGET_DEVICES += WMD_7621Ultra_12816

define Device/WMD_7621A_25616
  $(Device/uimage-lzma-loader)
  IMAGE_SIZE := 16064k
  DEVICE_VARIANT := 16M
  DEVICE_MODEL := WMD-7621A-25616
  SUPPORTED_DEVICES += Wooya,wms-7621a
endef
TARGET_DEVICES += WMD_7621A_25616


define Device/WMD_7621A_25664
  $(Device/uimage-lzma-loader)
  IMAGE_SIZE := 65216k
  DEVICE_VARIANT := 64M
  DEVICE_MODEL := WMD-7621A-25664
  SUPPORTED_DEVICES += Wooya,wms-7621a
endef
TARGET_DEVICES += WMD_7621A_25664

