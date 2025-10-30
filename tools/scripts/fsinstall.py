#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Dehuang.Wu
# Copyright (C) 2021-2024 ArtInChip Technology Co., Ltd

import os
import re
import sys
import shutil
import platform
import argparse


def fs_mkdir(path):
    dirname = os.path.dirname(path)
    if len(path) > 0 and path != dirname:
        fs_mkdir(dirname)
        if os.path.exists(path) is False:
            os.mkdir(path)


def fs_copy(srcfile, dstfile):
    with open(srcfile, 'rb') as sf:
        with open(dstfile, 'wb') as df:
            df.write(sf.read())


def clean_files(path_list, outpath):
    paths = path_list.split(',')
    for p in paths:
        if len(outpath) > 0:
            op = os.path.join(outpath, p)
            if os.path.exists(op):
                shutil.rmtree(op, ignore_errors=True)
                continue
        if os.path.exists(p):
            shutil.rmtree(p, ignore_errors=True)


def install_files(srcpath, dstpath):
    if os.path.exists(srcpath) is False:
        print('src is not exist')
        os.exit(1)
    if os.path.isdir(srcpath):
        fs_mkdir(dstpath)
        root_path = srcpath
        for root, dirs, files in os.walk(srcpath):
            for fn in files:
                fsrc = os.path.join(root, fn)
                rpath = fsrc.replace(srcpath, '')
                fdst = os.path.join(dstpath, rpath)
                fs_mkdir(os.path.dirname(fdst))
                fs_copy(fsrc, fdst)
    else:
        # cp file
        if dstpath.endswith('/') or dstpath.endswith('\\'):
            dstpath = os.path.join(dstpath, os.path.basename(srcpath))
        fs_mkdir(os.path.dirname(dstpath))
        fs_copy(srcpath, dstpath)


def main(args):
    if args.clean is not None:
        outpath = ''
        if args.sdkout:
            outpath = os.path.join(args.sdkout)
        clean_files(args.clean, outpath)
        return

    # Check SDK output directory first
    if args.dst is None:
        print('Error, please provide install dst')
        sys.exit(1)
    if args.src is None:
        print('Error, please provide install src')
        sys.exit(1)
    srcpath = args.src
    dstpath = args.dst
    if args.sdkout:
        dstpath = os.path.join(args.sdkout, args.dst)
    install_files(srcpath, dstpath)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--sdkout", type=str,
                        help="SDK output directory")
    parser.add_argument("-c", "--clean", type=str,
                        help="Clean directory")
    parser.add_argument("-s", "--src", type=str,
                        help="Source file or directory")
    parser.add_argument("-d", "--dst", type=str,
                        help="Destination path")
    args = parser.parse_args()

    main(args)
