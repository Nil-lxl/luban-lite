#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd
# Xiong Hao <hao.xiong@artinchip.com>
#
# Tool to update defconfig file
#

import pexpect
import os, sys


def config_update():
    child = pexpect.spawn('scons --menuconfig')
    # child.logfile = sys.stdout.buffer

    try:
        index = child.expect('ArtInChip Luban-Lite SDK Configuration', timeout=10)
        if index == 0:
            child.send('\x1b')
            child.send('\x1b')
        if index == 1:
            print("Wait for string 'ArtInChip Luban-Lite SDK Configuration' timeout")
            sys.exit(-1)


        index = child.expect(['Do you wish to save your new configuration',
                            'End of the configuration', pexpect.TIMEOUT], timeout=10)
        if index == 0:
            child.send('\r\n')
        if index == 2:
            print("Wait for string 'End of the configuration' timeout")
            sys.exit(-1)

        child.expect(pexpect.EOF)
        # print(child.before)
    except pexpect.ExceptionPexpect as e:
        print("Auto update config Error")
        sys.exit(-1)
    finally:
        # print("Auto update config finish")
        child.close()


if __name__ == '__main__':
    config_update()
