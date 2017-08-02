#!/bin/bash
export CROSS_COMPILE=arm-linux-gnueabihf-
export ARCH=arm

# Halt upon errors
set -e

# Make all tools
make -C ./dev-tools/flash_contents/flash_img_create/
make -C ./dev-tools/flash_contents/flash_img_obj_create/
make -C ./dev-tools/flash_contents/flash_img_print/

export HAL_TOP=$PWD/src/HAL

# Build U-Boot
cd ../../
make alpine_hw29765235p0p512p1024p4x4p4x4_32_db_config
make V=1

# Copy U-Boot binary to the image preparation input directory
cp u-boot.bin tools/al_boot_v_1_65_1/input/

# Compile the Device Tree
cd tools/al_boot_v_1_65_1/input/
dtc -o alpine_db.dtb -O dtb alpine_hw29765235p0p512p1024p4x4p4x4_32_db.dts
cd ../

# Generate the images
./dev-tools/flash_contents/scripts/alpine_db_spi_main/create.sh ./input ./output

# Generate a recovery image
DUMMY_STG2=`mktemp`
echo dummy > $DUMMY_STG2
./recovery/boot_image_create.sh $DUMMY_STG2 ./recovery/stage3.bin 256k ./input/u-boot.bin 0 ./output/boot.img.recovery.tmp
dd if=./output/boot.img.recovery.tmp of=./output/boot.img.recovery.tmp.padded bs=768k conv=sync > /dev/null 2>&1
dd if=./input/alpine_db.dtb of=./output/alpine_db.dtb.padded bs=64k conv=sync > /dev/null 2>&1
cat ./output/boot.img.recovery.tmp.padded ./output/alpine_db.dtb.padded > ./output/boot.img.recovery.tmp.withdt
dd if=./output/boot.img.recovery.tmp.withdt of=./output/boot.img.recovery bs=1 skip=12292 > /dev/null 2>&1
rm ./output/boot.img.recovery.tmp
rm ./output/boot.img.recovery.tmp.withdt
rm ./output/boot.img.recovery.tmp.padded
rm ./output/alpine_db.dtb.padded
rm $DUMMY_STG2

