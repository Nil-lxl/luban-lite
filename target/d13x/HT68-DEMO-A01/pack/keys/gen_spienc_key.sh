#!/bin/bash
#
# Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#
# Authors: Xiong Hao <hao.xiong@artinchip.com>
#

if [ ! -f "rsa_private_key.der" ] || [ ! -f "rsa_public_key.der" ];then
	openssl genrsa -out rsa_private_key.pem 2048
	openssl rsa -in rsa_private_key.pem -pubout -out rsa_public_key.pem
	openssl base64 -d -in rsa_public_key.pem -out rsa_public_key.der
	openssl base64 -d -in rsa_private_key.pem -out rsa_private_key.der
fi

if [ ! -f "set_aes_key.txt" ] || [ ! -f "set_nonce.txt" ];then
	openssl rand -hex 16 > set_aes_key.txt
	openssl rand -hex 8 > set_nonce.txt
fi

openssl dgst -md5 -binary -out rotpk.bin rsa_public_key.der

xxd -p -r set_aes_key.txt > spi_aes.key
xxd -p -r set_nonce.txt > spi_nonce.key
xxd -c 16 -i spi_aes.key > spi_aes_key.h
xxd -c 16 -i spi_nonce.key >> spi_aes_key.h
xxd -c 16 -i rotpk.bin >> spi_aes_key.h

cp spi_aes_key.h ../../../../../bsp/examples_bare/test-efuse/spi_aes_key.h
