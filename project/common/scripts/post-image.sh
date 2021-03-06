#!/bin/sh

. ${BASE_DIR}/project_locations # generated by make-buildroot.rb

GENIMAGE_CFG="${BASE_DIR}/genimage.cfg"
GENIMAGE_TMP="${BUILD_DIR}/genimage.tmp"

case "${2}" in
	--add-pi3-miniuart-bt-overlay)
	if ! grep -qE '^dtoverlay=pi3' "${BINARIES_DIR}/rpi-firmware/config.txt"; then
		echo "Adding 'dtoverlay=pi3-miniuart-bt' to config.txt (fixes ttyAMA0 serial console)."
		cat << __EOF__ >> "${BINARIES_DIR}/rpi-firmware/config.txt"

# fixes rpi3 ttyAMA0 serial console
dtoverlay=pi3-miniuart-bt
__EOF__
	fi
	;;
esac

rm -rf "${GENIMAGE_TMP}"; genimage \
	--rootpath "${TARGET_DIR}"     \
	--tmppath "${GENIMAGE_TMP}"    \
	--inputpath "${BINARIES_DIR}"  \
	--outputpath "${BINARIES_DIR}" \
	--config "${GENIMAGE_CFG}"

[[ $? -ne 0 ]] && exit $?

cat <<EOF

######   #######  #     #  #######   ###   
#     #  #     #  ##    #  #         ###   
#     #  #     #  # #   #  #         ###   
#     #  #     #  #  #  #  #####      #    
#     #  #     #  #   # #  #               
#     #  #     #  #    ##  #         ###   
######   #######  #     #  #######   ###   

Now you can use the 'make-finaldisk.sh' script to make the initial SD image.
Or, if this is already done, you can upgrade the root filesystem on your
running RPi.
EOF
