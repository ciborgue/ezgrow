CC1101D_VERSION = HEAD
CC1101D_SITE = $(call github,ciborgue,cc1101d,$(CC1101D_VERSION))
#CC1101D_SITE_METHOD = git
CC1101D_AUTORECONF = YES
CC1101D_DEPENDENCIES = wiringpi

$(eval $(autotools-package))