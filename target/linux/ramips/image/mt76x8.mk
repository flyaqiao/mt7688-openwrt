include ./common-tp-link.mk

DEFAULT_SOC := mt7628an

define Device/WMS-76X8A-6408
  IMAGE_SIZE := 7872k
  DEVICE_VARIANT := 8M
  DEVICE_MODEL := WMS-76X8A-6408
  SUPPORTED_DEVICES += Wooya,wms-76x8a
  DEVICE_PACKAGES := kmod-mt76x2 kmod-usb2 kmod-usb-ohci luci \
	drv_regopt reg \
	kmod-mt76 kmod-lib80211 \
	kmod-i2c-mt7628 kmod-sdhci-mt7620 kmod-usb-storage \
	quectel mt76x8_base
endef
TARGET_DEVICES += WMS-76X8A-6408

define Device/WMS-76X8A-12816
  IMAGE_SIZE := 16064k
  DEVICE_VARIANT := 16M
  DEVICE_MODEL := WMS-76X8A-12816
  SUPPORTED_DEVICES += Wooya,wms-76x8a
  DEVICE_PACKAGES := kmod-mt76x2 kmod-usb2 kmod-usb-ohci luci \
	drv_regopt reg \
	kmod-mt76 kmod-lib80211 \
	kmod-i2c-mt7628 kmod-sdhci-mt7620 kmod-usb-storage \
	quectel mt76x8_base
endef
TARGET_DEVICES += WMS-76X8A-12816

define Device/WMS-76X8A-12832
  IMAGE_SIZE := 32448k
  DEVICE_VARIANT := 32M
  DEVICE_MODEL := WMS-76X8A-12832
  SUPPORTED_DEVICES += Wooya,wms-76x8a
  DEVICE_PACKAGES := kmod-mt76x2 kmod-usb2 kmod-usb-ohci luci \
	drv_regopt reg \
	kmod-mt76 kmod-lib80211 \
	kmod-i2c-mt7628 kmod-sdhci-mt7620 kmod-usb-storage \
	quectel mt76x8_base
endef
TARGET_DEVICES += WMS-76X8A-12832
