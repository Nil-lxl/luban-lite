#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2024 ArtInChip Technology Co., Ltd
# Authors: dwj <weijie.ding@artinchip.com>
import os
import sys
import glob


def modify_pinmux_file(file_path):
    with open(file_path, 'r+') as f:
        header_start = 0
        header_end = 0
        need_add_header = 0
        need_add_size = 0
        struct_start = 0
        function_start = 0
        lines = f.readlines()

        if "#include <aic_utils.h>\n" not in lines:
            need_add_header = 1

        if "uint32_t aic_pinmux_config_size = ARRAY_SIZE(aic_pinmux_config);\n" not in lines:
            need_add_size = 1

        f.seek(0, 0)
        for line in lines:
            if need_add_header and header_end == 0:
                if line.find("#include") != -1 and header_start == 0:
                    header_start = 1
                if line.find("#include") == -1 and header_start == 1:
                    header_end = 1
                    f.write("#include <aic_utils.h>\n")
                    continue

            if line == "extern size_t __dtb_pos_f;\n":
                continue

            if line == "struct aic_pinmux\n":
                struct_start = 1
                brace_count = 0
                continue

            if line == "void aic_board_pinmux_init(void)\n":
                function_start = 1
                brace_count = 0
                continue

            if struct_start:
                brace_count = brace_count + line.count('{') - line.count('}')
                if (brace_count == 0):
                    struct_start = 0
                continue

            if function_start:
                brace_count = brace_count + line.count('{') - line.count('}')
                if (brace_count == 0):
                    function_start = 0
                continue

            f.write(line)

        f.truncate()
        if need_add_size:
            f.write("uint32_t aic_pinmux_config_size = ARRAY_SIZE(aic_pinmux_config);\n")


def modify_pinmux_files():
    if not os.path.exists("./tools/scripts/sdk_update.py"):
        print("Please run sdk_update.py in Luban-Lite Root directory!")
        return
    pinmux_files = glob.glob("./target/**/*pinmux.c", recursive=True)
    for file in pinmux_files:
        modify_pinmux_file(file)


if __name__ == "__main__":
    modify_pinmux_files()
