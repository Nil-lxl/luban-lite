#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2025 ArtInChip Technology Co., Ltd
# Authors: dwj <weijie.ding@artinchip.com>
import os
import sys
import json

seg_info = {}


def parse_elf_segment(src_file, prj_out_dir, toolchain_prefix):
    phdr = 0
    seg_section_map = 0
    segIdx = 0
    genSegIdx = 0
    bitmask = 0
    option = ""
    entry_point = ""

    SEG_INFO_TMP = prj_out_dir + '.seg_info_tmp'
    SEG_BIN_PREFIX = prj_out_dir + 'seg'
    objcopy_action = toolchain_prefix + 'objcopy -O binary'
    readelf_action = toolchain_prefix + 'readelf -lW '

    os.system("rm -rf " + prj_out_dir + "seg*.bin")
    os.system(readelf_action + src_file + ' > ' + SEG_INFO_TMP)

    with open(SEG_INFO_TMP, 'r') as f:
        # Read readelf command output line by line
        lines = f.readline()
        while lines:
            line_list = lines.split()

            if len(line_list) and line_list[0] == 'Type':
                phdr = 1
                lines = f.readline()
                continue

            if len(line_list) and line_list[0] == 'Entry' and line_list[1] == 'point':
                entry_point = line_list[2]

            if phdr:
                if len(line_list) and line_list[0] == 'LOAD' and int(line_list[4], 16):
                    bitmask |= (1 << segIdx)
                    seg_info.setdefault(genSegIdx, {})
                    seg_info[genSegIdx].setdefault("load", hex(int(line_list[2], 16)))
                    seg_info[genSegIdx].setdefault("entry", hex(int(entry_point, 16)))
                    genSegIdx += 1

                segIdx += 1

                if not len(line_list):
                    phdr = 0
                    segIdx = 0
                    genSegIdx = 0

            if len(line_list) and line_list[0] == 'Segment':
                seg_section_map = 1
                lines = f.readline()
                continue

            if seg_section_map:
                if bitmask & (1 << segIdx):
                    for i in range(1, len(line_list)):
                        option += ' -j ' + line_list[i]
                    seg_info[genSegIdx].setdefault("sections", option)
                    option = ""
                    genSegIdx += 1
                segIdx += 1

            lines = f.readline()

    # For xip-nor only, xip code must be in seg0
    if int(entry_point, 16) > 0x60000000:
        for i in range(0, len(seg_info)):
            if (seg_info[i]['load'] == entry_point and i):
                tmp = seg_info[0]
                seg_info[0] = seg_info[i]
                seg_info[i] = tmp

    for i in range(0, len(seg_info)):
        os.system(objcopy_action + seg_info[i]['sections'] + " " +
                  src_file + ' ' + SEG_BIN_PREFIX + str(i) + ".bin")

    os.system("rm " + SEG_INFO_TMP)


def parse_image_cfg(cfgfile):
    with open(cfgfile, "r") as f:
        lines = f.readlines()
        jsonstr = ""
        for line in lines:
            sline = line.strip()
            if sline.startswith("//"):
                continue
            slash_start = sline.find("//")
            if slash_start > 0:
                jsonstr += sline[0:slash_start].strip()
            else:
                jsonstr += sline

        jsonstr = jsonstr.replace(",}", "}").replace(",]", "]")
        cfg = json.loads(jsonstr)
    return cfg


def generate_its_head(depth, prj_out_dir):
    indent = depth * '\t'
    head_str = indent + "/dts-v1/;\n\n"
    head_str += indent + "/ {\n"
    head_str += indent + "\tdescription = \"Artinchip Luban-lite\";\n"
    head_str += indent + "\t#address-cells = <1>;\n\n"
    head_str += indent + "\timages {\n"

    cfg = parse_image_cfg(prj_out_dir + 'image_cfg.json')
    head_str += indent + "\t\tversion = \"" + cfg['image']['info']['version'] + "\";\n"

    return head_str


def generate_its_tail(depth):
    indent = depth * '\t'
    tail_str = indent + "};\n\n"
    tail_str += indent + "configurations {\n"
    tail_str += indent + "\tdefault = \"conf-1\";\n"
    tail_str += indent + "\tconf-1 {\n"
    tail_str += indent + "\t\tdescription = \"Luban-lite firmware\";\n"
    tail_str += indent + "\t\tfirmware ="
    for i in range(len(seg_info)):
        tail_str += " \"seg" + str(i) + "\""
        if i < len(seg_info) - 1:
            tail_str += ","
    tail_str += ";\n"
    tail_str += indent + "\t};\n"
    tail_str += indent + "};\n"
    tail_str += "};\n"
    return tail_str


def insert_seg_node(depth, seg_index):
    indent = depth * '\t'
    node_str = indent + "seg{} {{\n".format(seg_index)
    node_str += indent + "\tdescription = \"Artinchip segment {}\";\n".format(seg_index)
    node_str += indent + "\tdata = /incbin/(\"./seg{}.bin\");\n".format(seg_index)
    node_str += indent + "\ttype = \"firmware\";\n"
    node_str += indent + "\tarch = \"riscv\";\n"
    node_str += indent + "\tos = \"artinchip\";\n"
    node_str += indent + "\tload = <{}>;\n".format(seg_info[seg_index]["load"])
    node_str += indent + "\tentry = <{}>;\n".format(seg_info[seg_index]["entry"])
    node_str += indent + "\thash-1 {\n"
    node_str += indent + "\t\talgo = \"crc32\";\n"
    node_str += indent + "\t};\n"
    node_str += indent + "\thash-2 {\n"
    node_str += indent + "\t\talgo = \"md5\";\n"
    node_str += indent + "\t};\n"
    node_str += indent + "};\n"
    return node_str


def generate_its(output_file):
    file_path = output_file + '.its'

    with open(file_path, "w") as f:
        f.write(generate_its_head(0, sys.argv[2]))

        # insert seg0/1/2/3 etc. node
        for i in range(len(seg_info)):
            f.write(insert_seg_node(2, i))

        f.write(generate_its_tail(1))

if __name__ == "__main__":
    parse_elf_segment(sys.argv[1], sys.argv[2], sys.argv[3])
    output_file = sys.argv[1].replace(sys.argv[1][-4:], '_os')
    generate_its(output_file)

