#!/bin/bash

SD_SIZE=1536
MIB=$(( 1024 * 1024 ))

IMAGES="images"
SD_NAME="sdcard.bin"

checkSize() {
  SIZE=$(ls -l "$2" | sed -e 's/ [JFMASOND].*//;s/^.* //')
  if [[ $((SIZE % MIB)) -ne 0 ]]; then
    SIZE=$((1 + SIZE / MIB))
  else
    SIZE=$((SIZE / MIB))
  fi
  if [[ $SIZE -gt $1 ]]; then
    echo "${2}: exceeds the preset limit."
    exit 1
  fi
}
createImage() {
  dd if=/dev/zero of=$IMAGES/$SD_NAME ibs=$MIB count=$SD_SIZE 2> /dev/null
  echo "SD card image created for ${SD_SIZE}MiB."
}

BOOT_SZ=24
BOOT_FILE="$IMAGES/boot.vfat"
checkSize $BOOT_SZ $BOOT_FILE

ROOT_SZ=32
ROOT_FILE="$IMAGES/rootfs.squashfs"
checkSize $ROOT_SZ $ROOT_FILE

SITE_SZ=24
SITE_FILE="$IMAGES/site.ext4"
checkSize $SITE_SZ $SITE_FILE

createImage
fdisk $IMAGES/$SD_NAME > /dev/null <<EOF
o
n
p
1

+${BOOT_SZ}MiB
a
t
c
n
p
2

+${ROOT_SZ}MiB
n
p
3

+${ROOT_SZ}MiB
n
e


n

+${SITE_SZ}MiB
n


w
EOF
echo "SD card image partitioned."

for p in $(fdisk -l $IMAGES/$SD_NAME | sed -e "1,/^Device/d;s/^.*$SD_NAME/p/;s/*//" \
  -e 's/\([[:digit:]]\)[[:space:]]\{1,\}/\1,/;s/[[:space:]].*//'); do
  case "${p%%,*}" in
    "p1")
      echo "Writing boot @ ${p##*,} .."
      dd if=$BOOT_FILE of=$IMAGES/$SD_NAME bs=512 seek=${p##*,} conv=notrunc
    ;;
    "p2")
      echo "Writing root @ ${p##*,} .."
      dd if=$ROOT_FILE of=$IMAGES/$SD_NAME bs=512 seek=${p##*,} conv=notrunc
    ;;
    "p5")
      echo "Writing site @ ${p##*,} .."
      dd if=$SITE_FILE of=$IMAGES/$SD_NAME bs=512 seek=${p##*,} conv=notrunc
    ;;
    "p6")
      echo "Truncating disk image @ ${p##*,} .."
      echo "REPLACE THIS ONE4860d7efb8dd2e91e8ac0df7ce9f3063" | \
        dd of=$IMAGES/$SD_NAME bs=512 seek=${p##*,} count=1 conv=sync,fsync
    ;;
    *)
      echo "Skipping partition .."
  esac
done
