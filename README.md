This directory should be pointed to by BR2_EXTERNAL. But don't worry,
initial setup script will do it for you.

Usage pattern is as follows (building for RPi3):
	cd $HOME
	git clone git://git.buildroot.net/buildroot
	git clone https://github.org/ciborgue/ezgrow
	mkdir work
	cd work
	~/ezgrow/scripts/make-buildroot.rb ~/buildroot raspberrypi3
	make
	~/ezgrow/scripts/make-finaldisk.sh

Once completed, copy card image do the actual SD card, insert and booti
	(assuming /dev/sdb is your SD card, be careful!):
	sudo dd if=images/sdcard.img of=/dev/sdb obs=4k conv=fsync
