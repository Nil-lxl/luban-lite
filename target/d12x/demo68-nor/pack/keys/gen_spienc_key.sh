#!/bin/bash
#
# Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#
# Authors: Xiong Hao <hao.xiong@artinchip.com>
#

if [ ! -f "set_aes_key.txt" ];then
	openssl rand -hex 16 > set_aes_key.txt
fi

xxd -p -r set_aes_key.txt > spi_aes.key
xxd -c 16 -i spi_aes.key > spi_aes_key.h

cat spi_aes.key > all.key
crc32 spi_aes.key all.key > crc32.txt

cp spi_aes_key.h ../../../../../bsp/examples_bare/test-efuse/spi_aes_key.h
