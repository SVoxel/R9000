#
# Makefile for the linux squashfs routines.
#

KBUILD_CFLAGS := $(filter-out -O%,$(KBUILD_CFLAGS)) -O3

obj-$(CONFIG_SQUASHFS) += squashfs.o
squashfs-y += block.o cache.o dir.o export.o file.o fragment.o id.o inode.o
squashfs-y += namei.o super.o symlink.o decompressor.o
squashfs-$(CONFIG_SQUASHFS_XATTR) += xattr.o xattr_id.o
squashfs-$(CONFIG_SQUASHFS_LZO) += lzo_wrapper.o
squashfs-$(CONFIG_SQUASHFS_XZ) += xz_wrapper.o
squashfs-$(CONFIG_SQUASHFS_ZLIB) += zlib_wrapper.o
