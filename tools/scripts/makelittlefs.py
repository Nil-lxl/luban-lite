#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Dehuang.Wu
# Copyright (C) 2021-2025 ArtInChip Technology Co., Ltd

import os
import re
import sys
import platform
import argparse
import subprocess


def run_cmd(cmdstr):
    print(cmdstr)
    cmd = cmdstr.split(' ')
    ret = subprocess.run(cmd, stdout=subprocess.PIPE)
    if ret.returncode != 0:
        sys.exit(1)


def mkimage_get_resource_size(srcdir, block_siz):
    total_size = 0
    root_path = srcdir
    for root, dirs, files in os.walk(srcdir):
        for fn in files:
            fpath = os.path.join(root, fn)
            size = os.path.getsize(fpath)
            size = block_siz * int(round((size + block_siz - 1) / block_siz))
            total_size += size
    return total_size


def mkimage_get_mtdpart_size(outfile):
    imgname = os.path.basename(outfile)
    partlist = os.path.join(os.path.dirname(outfile), 'partition_file_list.h')
    size = 0
    if not os.path.exists(partlist):
        return 0
    with open(partlist) as f:
        lines = f.readlines()
        for ln in lines:
            name = ln.split(',')[1].replace('"', '').replace('*', '')
            if imgname == name or imgname in name:
                size = int(ln.split(',')[2])
                return size
    print('Image {} is not used in any partition'.format(imgname))
    print('please check your project\'s image_cfg.json')
    return size


def main(args):
    inputdir = args.inputdir
    # First priority is pack folder in the same directory with output file
    inputdir_1st = os.path.join(os.path.dirname(args.outfile), inputdir)
    if os.path.exists(inputdir_1st):
        inputdir = inputdir_1st
    if os.path.exists(inputdir) is False:
        print('Warning: inputdir {} is not exist, use a empty one.'.format(inputdir))
        inputdir = os.path.dirname(args.outfile) + '/empty'
        os.makedirs(inputdir, exist_ok=True)
    print(inputdir)
    if args.imgsize is not None:
        imgsize = int(args.imgsize)
    else:
        imgsize = mkimage_get_mtdpart_size(args.outfile)

    if imgsize == 0:
        imgsize = mkimage_get_resource_size(inputdir, int(args.blocksize))
        # Add some free space
        imgsize += (4 * int(args.blocksize))
    cmdstr = args.tooldir
    if platform.system() == 'Linux':
        cmdstr += 'mklittlefs '
    elif platform.system() == 'Windows':
        cmdstr += 'mklittlefs.exe '
    cmdstr += '-c {} '.format(inputdir)
    cmdstr += '-b {} '.format(args.blocksize)
    cmdstr += '-p {} '.format(args.pagesize)
    cmdstr += '-s {} '.format(imgsize)
    cmdstr += '{}'.format(args.outfile)
    run_cmd(cmdstr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--inputdir", type=str,
                        help="input directory")
    parser.add_argument("-o", "--outfile", type=str,
                        help="output file")
    parser.add_argument("-p", "--pagesize", type=str,
                        help="page size")
    parser.add_argument("-b", "--blocksize", type=str,
                        help="block size")
    parser.add_argument("-t", "--tooldir", type=str,
                        help="tool directory")
    parser.add_argument("-g", "--imgsize", type=str,
                        help="image size")
    args = parser.parse_args()
    main(args)
