#!/bin/sh

if [ $# != 6 ]; then
	echo Syntax error!
	echo Usage: boot_image_create.sh \<stage 2 image\> \<stage 3 image\> \<padded size\> \<u-boot image\> \<padded size\> \<output: boot image\>
	echo
	echo "\"stage 2 image\" is generated using the tool \"tools/stage2_image_create/stage2_image_create\""
	echo "\"stage 3 image\" is stage 3 executable in binary form"
	echo "\"padded size\" is the size to which the pre-boot image shall be padded (0 - no padding, e.g. 128k)"
	echo "\"u-boot image\" is executable in binary form"
	echo "\"padded size\" is the size to which the boot image shall be padded (0 - no padding, e.g. 512k)"
	echo
	echo Example: ./tools/scripts/boot_image_create.sh ./stage2/release/stage2.img ./uboot/release/uboot.img 128k u-boot.bin 512k ./boot_image.img
	echo
	exit 1
fi

if [ ! -f $1 ]; then
	echo "Stage 2 image input file not found!"
	exit 2
fi

if [ ! -f $2 ]; then
	echo "Stage 3 image input file not found!"
	exit 3
fi

if [ ! -f $4 ]; then
	echo "U-Boot image input file not found!"
	exit 4
fi

export stage2Image=$1
export stage3Image=$2
export paddingSize1=$3
export ubootImage=$4
export paddingSize2=$5
export bootImage=$6

export toolsDir=`dirname $0`/../

export stage2Padded=`mktemp`
export stage3ImageSize=`mktemp`
export prebootImage=`mktemp`
export ubootImageSize=`mktemp`

# Pad stage 2 to 12KB
dd if=${stage2Image} of=${stage2Padded} bs=12k conv=sync > /dev/null 2>&1

# Generate a binary file which contains stage 3 file size in 32 bits little endian
ls -nlH ${stage3Image} | awk '{printf("%00000000: %02x %02x %02x %02x\n", and($5, 0xff), and(rshift($5, 8), 0xff), and(rshift($5, 16), 0xff), and(rshift($5, 24), 0xff));}' | xxd -r > ${stage3ImageSize}

# Concatenate padded stage 2, stage 3 image size, and stage 3 image
cat ${stage2Padded} ${stage3ImageSize} ${stage3Image} > ${prebootImage}

# Pad the image, if required
if [ ${paddingSize1} != "0" ]; then
dd if=${prebootImage} of=${prebootImage}.padded bs=${paddingSize1} conv=sync > /dev/null 2>&1
rm -f ${prebootImage}
mv ${prebootImage}.padded ${prebootImage}
fi

# Generate a binary file which contains U-Boot file size in 32 bits little endian
ls -nlH ${ubootImage} | awk '{printf("%00000000: %02x %02x %02x %02x\n", and($5, 0xff), and(rshift($5, 8), 0xff), and(rshift($5, 16), 0xff), and(rshift($5, 24), 0xff));}' | xxd -r > ${ubootImageSize}

# Concatenate preboot Image, U-Boot image size, and U-Boot image
cat ${prebootImage} ${ubootImageSize} ${ubootImage} > ${bootImage}

# Pad the image, if required
if [ ${paddingSize2} != "0" ]; then
dd if=${bootImage} of=${bootImage}.padded bs=${paddingSize2} conv=sync > /dev/null 2>&1
rm -f ${bootImage}
mv ${bootImage}.padded ${bootImage}
fi

# Remove intermediate files
rm -f ${stage2Padded}
rm -f ${stage3ImageSize}
cp ${prebootImage} prebootImage
rm -f ${prebootImage}
rm -f ${ubootImageSize}
