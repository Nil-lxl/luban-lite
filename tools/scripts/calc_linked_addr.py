#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd
# Xiong Hao <hao.xiong@artinchip.com>
#
# Tool to calculate bootloader linked addr from .config file
#

import os
from menuconfig import get_config_val, get_text_base


def calc_link_addr(infile, config, outfile):
    mem_auto_enable = 'CONFIG_AIC_BOOTLOADER_MEM_AUTO'
    text_base_config = 'CONFIG_AIC_BOOTLOADER_TEXT_BASE'
    text_load_config = 'CONFIG_AIC_BOOTLOADER_LOAD_BASE'

    if get_config_val(config, mem_auto_enable) == 'y':
        text_base = hex(get_text_base(config))
        load_base = hex(int(text_base, 16) - 0x100)
    else:
        text_base = get_config_val(config, text_base_config)
        load_base = hex(int(text_base, 16) - 0x100)
    with open(infile, 'r') as f:
        cfg = f.read()

    cfg = cfg.replace(text_base_config, text_base)
    cfg = cfg.replace(text_load_config, load_base)

    with open(outfile, 'w') as f:
        f.write(cfg)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description='Tool to calculate bootloader linked addr from .config file')
    parser.add_argument('-i', '--infile', type=str, help='input image_cfg.json file')
    parser.add_argument('-c', '--config', type=str, help='bootloader defconfig file')
    parser.add_argument('-o', '--outfile', type=str, help='output image_cfg.jaon file')
    args = parser.parse_args()
    if args.infile is None:
        print('Error, option --infile is required.')
        sys.exit(1)
    if args.config is None:
        print('Error, option --config is required.')
        sys.exit(1)
    if args.outfile is None:
        outname = '.' + os.path.basename(img) + '.tmp'
        args.outfile = os.path.dirname(img) + '/' + outname
        sys.exit(1)

    calc_link_addr(args.infile, args.config, args.outfile)
