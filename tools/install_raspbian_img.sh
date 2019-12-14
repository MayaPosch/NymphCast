#!/bin/sh

# install_raspbian_img.sh - Install cross-compiled NymphCast server on a fresh Raspbian .img file.

IMG_FILE="2019-09-26-raspbian-buster.img"

# Open img file.
sudo kpartx -l $IMG_FILE
sudo kpartx -av $IMG_FILE

# Mount the partition.
# First partition (loop0p1) is the boot partition, the second (loop0p2) is the system partition.
sudo mount -o loop /dev/mapper/loop0p2 /mnt/ 

# Copy files onto the partition.


# Clean-up
sudo umount /mnt
sudo kpartx -d $IMG_FILE
