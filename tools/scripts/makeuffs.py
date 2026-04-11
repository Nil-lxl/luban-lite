#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Dehuang.Wu
# Copyright (C) 2021-2024 ArtInChip Technology Co., Ltd

import os
import re
import sys
import platform
import argparse
import subprocess


def run_cmd(cmdstr):
    # print(cmdstr)
    cmd = cmdstr.split(' ')
    ret = subprocess.run(cmd, stdout=subprocess.PIPE)
    if ret.returncode != 0:
        sys.exit(1)


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
        print('{} is not exist.'.format(inputdir))
        return
    partsize = mkimage_get_mtdpart_size(args.outfile)
    blksiz = int(args.pagesize) * int(args.blockpagecount)
    partblkcnt = int(partsize / blksiz)
    cmdstr = args.tooldir
    if platform.system() == 'Linux':
        cmdstr += 'mkuffs '
    elif platform.system() == 'Windows':
        cmdstr += 'mkuffs.exe '
    cmdstr += '-t {} '.format(partblkcnt)
    cmdstr += '-p {} '.format(args.pagesize)
    cmdstr += '-b {} '.format(args.blockpagecount)
    cmdstr += '-s {} '.format(args.oobsize)
    cmdstr += '-x auto -o 0 '
    cmdstr += '-f {} '.format(args.outfile)
    cmdstr += '-d {}'.format(inputdir)
    run_cmd(cmdstr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--inputdir", type=str,
                        help="input directory")
    parser.add_argument("-o", "--outfile", type=str,
                        help="output file")
    parser.add_argument("-g", "--blockpagecount", type=str,
                        help="page count in one block")
    parser.add_argument("-p", "--pagesize", type=str,
                        help="page size")
    parser.add_argument("-s", "--oobsize", type=str,
                        help="page oob size")
    parser.add_argument("-t", "--tooldir", type=str,
                        help="tool directory")
    args = parser.parse_args()
    main(args)
