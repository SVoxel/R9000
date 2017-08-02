CONFIG_QCA_SINGLE_IMG_TREEISH = 5385612103af301a07b51f883ae8da68e6da995c

export CONFIG_QCA_SINGLE_IMG_TREEISH

single_img_dep = u-boot.mbn

define BuildSingleImg
	$(MAKE) -C tools/qca_single_img/ patch_clean
	cp -R board/"$(BOARDDIR)"/qca_single_img/./ \
			tools/qca_single_img/
	$(MAKE) -C tools/qca_single_img/

	@ ### Steps described in QSDK release notes ###
	cp u-boot.mbn \
			tools/qca_single_img/qsdk-chipcode/common/build/ipq/openwrt-ipq806x-u-boot.mbn
	cd tools/qca_single_img/qsdk-chipcode/common/build && \
			python update_common_info.py

	cp tools/qca_single_img/qsdk-chipcode/common/build/bin/nand-ipq806x-single.img $@
endef
