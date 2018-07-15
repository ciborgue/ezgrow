ADSADCD_VERSION = HEAD
ADSADCD_SITE = $(call github,ciborgue,adsadcd,$(ADSADCD_VERSION))
#ADSADCD_SITE_METHOD = git
#ADSADCD_SITE = /var/lib/libvirt/home/builduser/adsadcd
#ADSADCD_SITE_METHOD = local
ADSADCD_AUTORECONF = YES
ADSADCD_DEPENDENCIES = wiringpi

$(eval $(autotools-package))
