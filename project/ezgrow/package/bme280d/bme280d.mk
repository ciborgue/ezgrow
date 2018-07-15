#BME280D_VERSION = a05bec8
BME280D_VERSION = HEAD
BME280D_SITE = $(call github,ciborgue,bme280d,$(BME280D_VERSION))
#BME280D_SITE_METHOD = git
#BME280D_SITE = /var/lib/libvirt/home/builduser/bme280d
#BME280D_SITE_METHOD = local
BME280D_AUTORECONF = YES
BME280D_DEPENDENCIES = wiringpi

$(eval $(autotools-package))
