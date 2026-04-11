#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2024 ArtInChip Technology Co., Ltd
# zrq <ruiqi.zheng@artinchip.com>

import os
import sys
import re
import json
import subprocess

aic_scripts = sys.argv[0]
aic_system = sys.argv[1]
aic_root = os.path.normpath(sys.argv[2])
aic_pack_dir = os.path.normpath(sys.argv[3])
prj_out_dir = os.path.normpath(sys.argv[4])


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


def generate_version(cfgfile):

    cfg = parse_image_cfg(cfgfile)
    head_str = "version = \"" + cfg['image']['info']['version'] + "\";\n"

    return head_str


def modify_ota_cfg(cfgfile):

    with open(cfgfile, "r") as f:
        lines = f.readlines()

    if lines:
        lines[2] = generate_version(aic_pack_dir + '/image_cfg.json')

    with open(cfgfile, "w") as f:
        f.writelines(lines)


def parse_update_file(cfgfile):

    file_list = []

    with open(cfgfile, 'r') as f:
        content = f.read()

    matches = re.findall(r'\[(.*?)\]\s*([^[]+)', content, re.DOTALL)

    for match in matches:
        section_name = match[0].strip()
        section_content = match[1].strip()

        if section_name == 'file':
            files = section_content.splitlines()
            for file_entry in files:
                file_info = file_entry.split(':')
                if len(file_info) >= 1:
                    file_name = file_info[0].strip()
                    file_list.append(file_name)

    return file_list


def get_file_size(cfgfile):
    if not os.path.exists(cfgfile):
        return f"Error: File '{cfgfile}' does not exist."

    size = os.path.getsize(cfgfile)

    return size


def find_ota_info_position(cfgfile):
    try:
        with open(cfgfile, 'r+b') as file:
            while True:
                line = file.readline()
                if not line:
                    break
                index = line.find(b'ota_info.bin')
                if index != -1:
                    return file.tell() - len(line) + index + len(b'ota_info.bin')
            print(f"Error: 'ota_info.bin.' string not found in the file.")
    except FileNotFoundError:
        print(f"Error: File '{cfgfile}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")


def parse_and_fill_ota_size(cfgfile, value):
    # Fixed at a size of 512 bytes
    ota_info_size = 512

    try:
        with open(cfgfile, 'r+b') as file:
            pos = find_ota_info_position(cfgfile)
            file.seek(pos)

            while True:
                content = file.read(ota_info_size)
                if not content:
                    break
                index = content.find(b'size = "')
                if index != -1:
                    start = index + len(b'size = "')
                    end = content.find(b'"', start)
                    if end != -1:
                        # Calculate the new size value
                        new_size_value = f'{value}'.ljust(end - start, ' ').encode('utf-8')
                        # Ensure the content is exactly 512 bytes
                        new_content = content[:start] + new_size_value + content[end:]
                        new_content = new_content[:ota_info_size]  # Trim to 512 bytes if necessary
                        # Write back to the file
                        file.seek(-len(content), 1)
                        file.write(new_content)
                        print(f"Successfully updated size to {cfgfile}")
                        return

            print("Error: 'size' not found in the binary file.")
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")


def generate_cpio_bin(datadir, cfgfile, bindir):

    ota_info_file = datadir + cfgfile

    if not os.path.exists(ota_info_file):
        return f"No need to generate '{ota_info_file}' binary file."

    outfile = datadir + "/ota_info.bin"

    # not need for a larger size
    size = 512

    mkenvcmd = os.path.join(bindir, "mkenvimage")
    if os.path.exists(mkenvcmd) is False:
        mkenvcmd = "mkenvimage"
    if sys.platform == "win32":
        mkenvcmd += ".exe"

    cmd = [mkenvcmd, "-s", str(size), "-o", outfile, ota_info_file]

    ret = subprocess.run(cmd, subprocess.PIPE)
    if ret.returncode != 0:
        sys.exit(1)


os.chdir(prj_out_dir)

default_bin_root = os.path.dirname(sys.argv[0])
if sys.platform.startswith("win"):
    default_bin_root = os.path.dirname(sys.argv[0]) + "/"

# update the version
modify_ota_cfg(aic_pack_dir + '/ota-subimgs.cfg')

# copy to the output dir
os.system('cp ' + aic_pack_dir + '/ota-subimgs.cfg ' + prj_out_dir)

# generate the ota-subimgs.cfg to binary file
generate_cpio_bin(prj_out_dir, '/ota-subimgs.cfg', default_bin_root)

# generate the ota.cpio file
cat_cmd = '\n'.join(parse_update_file('ota-subimgs.cfg'))

with open('ota-temp.cfg', 'w') as temp_file:
    temp_file.write(cat_cmd)

if aic_system == 'Linux':
    os.system('cat ota-temp.cfg | cpio -ov -H crc > ota.cpio')
elif aic_system == 'Windows':
    cpio_tool_dir = os.path.join(aic_root, 'tools', 'scripts', 'cpio.exe')
    os.system('type ota-temp.cfg | ' + cpio_tool_dir + ' -ov -H crc > ota.cpio')

os.remove('ota-temp.cfg')

# modify the size to ota.cpio
parse_and_fill_ota_size('ota.cpio', get_file_size('ota.cpio'))

os.chdir(aic_root)
