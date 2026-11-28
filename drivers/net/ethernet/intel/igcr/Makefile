# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2018 Intel Corporation

obj-$(CONFIG_IGCR) += igcr.o

igcr-y := igc_main.o igc_mac.o igc_i225.o igc_base.o igc_nvm.o igc_phy.o \
	  igc_diag.o igc_ethtool.o igc_ptp.o igc_dump.o igc_tsn.o igc_xdp.o
igcr-$(CONFIG_IGCR_LEDS) += igc_leds.o
