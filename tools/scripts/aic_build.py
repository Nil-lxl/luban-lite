#!/usr/bin/env python2
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd

import os
import re
import sys
import glob
import platform
import shutil
import tarfile
import hashlib
import operator
from SCons.Script import *
# from building import *

COLOR_BEGIN = "\033["
COLOR_RED = COLOR_BEGIN + "41;37m"
COLOR_YELLOW = COLOR_BEGIN + "43;30m"
COLOR_WHITE = COLOR_BEGIN + "47;30m"
COLOR_END = "\033[0m"


def pr_err(string):
    print(COLOR_RED + '*** ' + string + COLOR_END)


def pr_info(string):
    print(COLOR_WHITE + '>>> ' + string + COLOR_END)


def pr_warn(string):
    print(COLOR_YELLOW + '!!! ' + string + COLOR_END)


def do_system(cmd):
    print('{}'.format(cmd))
    return os.system(cmd)


def do_pipe(cmd):
    p = os.popen(cmd)
    return p.readlines()[0]

def strip_component(member, strip_components):
    parts = member.name.split('/')
    if len(parts) > strip_components:
        member.name = '/'.join(parts[strip_components:])
        if member.islnk():
            link_parts = member.linkname.split('/')
            if len(link_parts) > strip_components:
                member.linkname = '/'.join(link_parts[strip_components:])
        return member
    else:
        return None

def tar(abs_f, datadir, dirname):
    with tarfile.open(abs_f, 'w:gz') as t:
        t.add(os.path.join(datadir, dirname), dirname);

def untar(tar_f, tardir, strip_components):
    with tarfile.open(tar_f, 'r:gz') as t:
        members = t.getmembers()
        members = [strip_component(member, strip_components) for member in members]
        members = [member for member in members if member]
        t.extractall(tardir, members)

def get_config(filename, key):
    if not os.path.exists(filename):
        return None
    for line in open(filename):
        line = line.strip()
        if not line or line[0] == '#':
            continue
        v = line.split('=')
        if v[0] == key:
            return v[1]
    return None


def get_input(prompt):
    if sys.version_info[0] < 3:
        # Python 2.X
        return raw_input(prompt)
    else:
        # Python 3.X
        return input(prompt)


def get_prj_defconfig(aic_root):
    config_file = os.path.join(aic_root, '.config')
    if not os.path.exists(config_file):
        return None
    key_defconfig = 'CONFIG_PRJ_DEFCONFIG_FILENAME'
    key_chip = 'CONFIG_PRJ_CHIP'
    key_board = 'CONFIG_PRJ_BOARD'
    key_kernel = 'CONFIG_PRJ_KERNEL'
    key_app = 'CONFIG_PRJ_APP'
    defconfig = get_config(config_file, key_defconfig)
    if defconfig:
        defconfig = defconfig.replace('"', '')
    else:
        prj_chip = get_config(config_file, key_chip)
        prj_board = get_config(config_file, key_board)
        prj_kernel = get_config(config_file, key_kernel)
        prj_app = get_config(config_file, key_app)
        defconfig = prj_chip + '_' + prj_board + '_' + prj_kernel + '_' + prj_app + '_defconfig'
    return defconfig


def list_defconfig(aic_root, noboot=False):
    v = []
    maxlen = 0
    path = os.path.join(aic_root, 'target', 'configs')
    for root, dirs, files in os.walk(path):
        if root != path:
            break
        for f in files:
            if noboot and f.endswith('_defconfig') and 'bootloader' not in f:
                v.append(f)
                if len(f) > maxlen:
                    maxlen = len(f)
            if noboot is False and f.endswith('_defconfig'):
                v.append(f)
                if len(f) > maxlen:
                    maxlen = len(f)
    v.sort()
    return v, maxlen


def mk_kconfig_prj(aic_root, filename):
    # generate .Kconfig.prj
    try:
        config = open(filename, 'r')
    except Exception as e:
        print('open config: {} failed: {}'.format(filename, e))
        return
    prj_chip = None
    prj_board = None
    prj_kernel = None
    prj_app = None
    for line in config:
        line = line.lstrip(' ').replace('\n', '').replace('\r', '')

        if len(line) == 0:
            continue
        if line.startswith('CONFIG_PRJ_CHIP'):
            prj_chip = line.split('=')[1].replace('"', '').strip()
        if line.startswith('CONFIG_PRJ_BOARD'):
            prj_board = line.split('=')[1].replace('"', '').strip()
        if line.startswith('CONFIG_PRJ_KERNEL'):
            prj_kernel = line.split('=')[1].replace('"', '').strip()
        if line.startswith('CONFIG_PRJ_APP'):
            prj_app = line.split('=')[1].replace('"', '').strip()

    if prj_chip is None or prj_board is None or prj_kernel is None or prj_app is None:
        return

    with open(".Kconfig.prj", "w") as f:
        f.write('source "bsp/artinchip/sys/{}/Kconfig.chip"\n'.format(prj_chip))
        f.write('source "target/{}/common/Kconfig.board"\n'.format(prj_chip))

        tmp = os.path.join(aic_root, "target", prj_chip, prj_board, "Kconfig.extend")
        if os.path.exists(tmp):
            f.write('source "target/{}/{}/Kconfig.extend"\n'.format(prj_chip, prj_board))

        f.write('source "kernel/{}/Kconfig"\n'.format(prj_kernel))
        f.write('source "application/{}/{}/Kconfig"\n'.format(prj_kernel, prj_app))
        if prj_kernel == 'rt-thread':
            f.write('source "tools/env/packages/packages/Kconfig"\n')


def update_project_env(aic_root):
    os.environ['PKGS_ROOT'] = os.path.join(aic_root, 'tools', 'env', 'packages')
    os.environ['PKGS_DIR'] = os.path.join(aic_root, 'tools', 'env', 'packages')
    os.environ['RTT_ROOT'] = os.path.join(aic_root, 'kernel', 'rt-thread/')
    os.environ['RTT_DIR'] = os.path.join(aic_root, 'kernel', 'rt-thread/')


def apply_defconfig(aic_root, defconfig):
    src = os.path.join(aic_root, 'target', 'configs', defconfig)
    if not os.path.exists(src):
        print('defconfig file {} is not exists!'.format(src))
        sys.exit(110)

    print('Load config from {}'.format(os.path.join('target', 'configs', defconfig)))
    os.system('echo ' + defconfig.replace('_defconfig', '') + ' > ' +
              os.path.join(aic_root, '.defconfig'))

    dst = os.path.join(aic_root, '.config')
    shutil.copy(src, dst)
    sys.path.append(os.path.join(aic_root, 'kernel/rt-thread/tools'))

    # Should generate .Kconfig.prj before apply defconfig, because project depends on this file.
    mk_kconfig_prj(aic_root, src)
    update_project_env(aic_root)
    pydefconfig = os.path.join(aic_root, 'kernel', 'rt-thread', 'tools',  'defconfig.py')
    defconf_cmd = 'python3 {} {}'.format(pydefconfig, src)
    os.system(defconf_cmd)
    from menuconfig import mk_rtconfig
    mk_rtconfig('.config')


def save_defconfig(aic_root):
    src = os.path.join(aic_root, '.config')
    if not os.path.exists(src):
        print('defconfig file {} is not exists!'.format(src))
        sys.exit(110)
    defconfig = get_prj_defconfig(aic_root)
    if defconfig:
        dst = os.path.join(aic_root, 'target', 'configs', defconfig)
        update_project_env(aic_root)
        pydefconfig = os.path.join(aic_root, 'kernel', 'rt-thread', 'tools',  'savedefconfig.py')
        defconf_cmd = 'python3 {} --out {}'.format(pydefconfig, dst)
        os.system(defconf_cmd)


def update_defconfig(aic_root):
    defconfig = get_prj_defconfig(aic_root)
    if defconfig:
        apply_defconfig(aic_root, defconfig)


# cmd-option: list defconfig without bootloader
def list_noboot_def_cmd(aic_root):
    AddOption('--list-noboot', dest='list_noboot_defconfig', action='store_true',
              default=False, help='to list all board defconfigs without bootloader')
    list_def = GetOption('list_noboot_defconfig')
    if list_def:
        configs, maxlen = list_defconfig(aic_root, noboot=True)
        index = 0
        print('Built-in configs:')
        for c in configs:
            d = c.replace('_defconfig', '')
            print('%3d. %-*s' % (index, maxlen, d))
            index += 1
        exit(0)


# cmd-option: apply defconfig without bootloader
def apply_noboot_def_cmd(aic_root):
    AddOption('--apply-noboot', dest='apply_notboot_defconfig', type='string',
              nargs=1, action='store',
              default="", help='to apply one board defconfig not for bootloader')
    apply_def = GetOption('apply_notboot_defconfig')
    if apply_def:
        if apply_def.isdigit():
            i = int(apply_def, 10)
            configs, maxlen = list_defconfig(aic_root, noboot=True)
            if i < len(configs):
                apply_def = configs[i]
        apply_defconfig(aic_root, apply_def)
        exit(0)


# cmd-option: list defconfig
def list_def_cmd(aic_root):
    AddOption('--list-def', dest='list_defconfig', action='store_true',
              default=False, help='to list all board defconfigs')
    list_def = GetOption('list_defconfig')
    if list_def:
        configs, maxlen = list_defconfig(aic_root, noboot=False)
        index = 0
        print('Built-in configs:')
        for c in configs:
            d = c.replace('_defconfig', '')
            print('%3d. %-*s' % (index, maxlen, d))
            index += 1
        exit(0)


# cmd-option: apply defconfig
def apply_def_cmd(aic_root):
    AddOption('--apply-def', dest='apply_defconfig', type='string',
              nargs=1, action='store',
              default="", help='to apply one board defconfig')
    apply_def = GetOption('apply_defconfig')
    if apply_def:
        if apply_def.isdigit():
            i = int(apply_def, 10)
            configs, maxlen = list_defconfig(aic_root)
            if i < len(configs):
                apply_def = configs[i]
        apply_defconfig(aic_root, apply_def)
        exit(0)


# cmd-option: save defconfig
def save_def_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--save-def', dest='save_defconfig', action='store_true',
              default=False, help='to save current board defconfig')
    save_def = GetOption('save_defconfig')
    if save_def:
        save_defconfig(aic_root)
        exit(0)


# cmd-option: call 'pkgs --update'
def pkgs_update_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--pkgs-update', dest='pkgs_update', action='store_true',
              default=False, help='to update packages online')
    update_def = GetOption('pkgs_update')
    if update_def:
        if platform.system() == 'Linux':
            env_py_path = os.path.join(aic_root, './tools/env/tools/scripts/env.py')
            os.system('python ' + env_py_path + ' package --update')
        elif platform.system() == 'Windows':
            os.system('pkgs --update')
        exit(0)


# cmd-option: project info
def show_info_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    config_file = os.path.join(aic_root, '.config')
    AddOption('--info', dest='prj_info', action='store_true',
              default=False, help='to show current project info')
    prj_info = GetOption('prj_info')
    if prj_info:
        # arch
        arch = 'unknown'
        if get_config(config_file, 'CONFIG_ARCH_RISCV32'):
            arch = 'riscv32'
        if get_config(config_file, 'CONFIG_ARCH_RISCV64'):
            arch = 'riscv64'
        if get_config(config_file, 'CONFIG_ARCH_CSKY'):
            arch = 'csky'
        # out dir
        prj_name = prj_defconfig.replace('_defconfig', '')
        prj_out_dir = 'output/' + prj_name
        # toolchain
        toolchain = 'toolchain/bin'
        if os.path.exists('toolchain/.ready'):
            chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
            sys.path.append(chip_path)
            import rtconfig
            toolchain = os.path.join(toolchain, rtconfig.PREFIX)
        toolchain_ver = get_config(config_file, 'CONFIG_PRJ_TOOLCHAIN_VER').replace('"', '')

        # summary
        print('    Target app: application/{}/{}'.format(prj_kernel, prj_app))
        print('   Target chip: {}'.format(prj_chip))
        print('   Target arch: {}'.format(arch))
        print('  Target board: target/{}/{}'.format(prj_chip, prj_board))
        print(' Target kernel: kernel/{}'.format(prj_kernel))
        print('Defconfig file: target/configs/{}'.format(prj_defconfig))
        print('Root directory: {}'.format(aic_root))
        print(' Out directory: {}'.format(prj_out_dir))
        print(' Toolchain ver: {}'.format(toolchain_ver))
        print('     Toolchain: {}'.format(toolchain))
        exit(0)


def add_board_get_chip(chips):
    while 1:
        index = 0
        print("Chip list:")
        for c in chips:
            print('%3d: %s' % (index, c))
            index += 1
        i = get_input("Select chip for new board(number): ")
        if i == 'q':
            exit(0)
        if not i.isdigit():
            print("Please enter the number!")
            continue
        i = int(i, 10)
        if i >= len(chips):
            print("Input number is invalid!")
            continue
        break
    return i


def add_board_get_defconfig(configs):
    while 1:
        index = 0
        print("Reference defconfig:(Create new board base on selected defconfig):")
        for c in configs:
            print('%3d: %s' % (index, c))
            index += 1
        i = get_input("Select reference defconfig for new board(number): ")
        if i == 'q':
            exit(0)
        if not i.isdigit():
            print("Please enter the number!")
            continue
        i = int(i, 10)
        if i >= len(configs):
            print("Input number is invalid!")
            continue
        break
    return i


def add_board_get_board_name():
    while 1:
        i = get_input("Input new board's name: ")
        i = i.strip()
        if i == 'q':
            exit(0)
        if '_' in i or ' ' in i:
            print("Input name can't contain '_' or ' ' !")
            continue
        if i != '':
            break
    return i


def add_board_get_app_name(app):
    while 1:
        i = get_input("Input new app's name: (default {}) ".format(app))
        i = i.strip()
        if i == 'q':
            exit(0)
        if '_' in i or ' ' in i:
            print("Input name can't contain '_' or ' ' !")
            continue
        if i == '':
            i = app
        break
    return i


def add_board_get_manufacturer_name():
    while 1:
        i = get_input("Input manufacturer's name: ")
        i = i.strip()
        if i == 'q':
            exit(0)
        break
    return i


# cmd-option: add board
def add_board_cmd(aic_root):
    AddOption('--add-board', dest='add_board', action='store_true',
              default=False, help='add new board')
    add_board = GetOption('add_board')
    if add_board:
        path = os.path.join(aic_root, 'target')
        # select chip
        chips = get_all_chip(path)
        i = add_board_get_chip(chips)
        chip = chips[i]
        print("{} \n".format(chip))
        # select reference board
        configs, maxlen = list_defconfig(aic_root)
        configs = list(filter(lambda x: chip in x and 'bootloader' not in x, configs))
        i = add_board_get_defconfig(configs)
        ref_cfg = configs[i]
        print("{} \n".format(ref_cfg))
        def_elements = ref_cfg.split('_')
        if len(def_elements) != 5:
            print("Error: {} is not conform to 'CHIP_BOARD_KERNEL_APP_defconfig' format!"
                  .format(ref_cfg))
            exit(0)
        board = def_elements[1]
        kernel = def_elements[2]
        app = def_elements[3]
        # input new board name
        i = add_board_get_board_name()
        new_board = i
        print("{} \n".format(new_board))
        # input new app name
        i = add_board_get_app_name(app)
        new_app = i
        print("{} \n".format(new_app))
        # input manufacturer name
        i = add_board_get_manufacturer_name()
        manuf_name = i
        print("{} \n".format(manuf_name))
        # copy xxx_defconfig
        src_def = os.path.join(aic_root, 'target', 'configs', ref_cfg)
        def_elements[1] = new_board
        def_elements[3] = new_app
        des_cfg = '_'.join(def_elements)
        des_def = os.path.join(aic_root, 'target', 'configs', des_cfg)
        print("Copy {} to {}".format(src_def, des_def))
        shutil.copy(src_def, des_def)
        file_data = ''
        with open(des_def, "r") as f:
            for line in f:
                if src_def in line:
                    line = line.replace(src_def, des_def)
                if board in line:
                    line = line.replace(board, new_board)
                if app in line:
                    line = line.replace(app, new_app)
                if 'CONFIG_AIC_CONSOLE_SYSNAME' in line:
                    line = 'CONFIG_AIC_CONSOLE_SYSNAME="{}"\n'.format(manuf_name)
                if 'CONFIG_FINSH_PROMPT_NAME' in line:
                    line = 'CONFIG_FINSH_PROMPT_NAME="{}"\n'.format(manuf_name)
                file_data += line
        with open(des_def, "w") as f:
            f.write(file_data)
        # copy xxx_baremetal_bootloader_defconfig
        def_elements[1] = board
        def_elements[2] = 'baremetal'
        def_elements[3] = 'bootloader'
        src_cfg = '_'.join(def_elements)
        src_def = os.path.join(aic_root, 'target', 'configs', src_cfg)
        def_elements[1] = new_board
        des_cfg = '_'.join(def_elements)
        des_def = os.path.join(aic_root, 'target', 'configs', des_cfg)
        if not os.path.exists(src_def):
            print("Warning: {} file is not exists \n".format(src_def))
        else:
            print("Copy {} to {}".format(src_def, des_def))
            shutil.copy(src_def, des_def)
            file_data = ''
            with open(des_def, "r") as f:
                for line in f:
                    if src_def in line:
                        line = line.replace(src_def, des_def)
                    if board in line:
                        line = line.replace(board, new_board)
                    file_data += line
            with open(des_def, "w") as f:
                f.write(file_data)
        # copy board
        src_d = os.path.join(aic_root, 'target', chip, board)
        des_d = os.path.join(aic_root, 'target', chip, new_board)
        src_d = os.path.normpath(src_d)
        des_d = os.path.normpath(des_d)
        if not os.path.exists(des_d):
            print("Copy {} to {}".format(src_d, des_d))
            if platform.system() == 'Linux':
                shutil.copytree(src_d, des_d)
            elif platform.system() == 'Windows':
                shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)
        des_file = os.path.join(des_d, 'pack', 'image_cfg.json')
        file_data = ''
        with open(des_file, "r") as f:
            for line in f:
                if board in line:
                    line = line.replace(board, new_board)
                file_data += line
        with open(des_file, "w") as f:
            f.write(file_data)
        # copy app
        if app != new_app:
            app_os = kernel
            src_d = os.path.join(aic_root, 'application', app_os, app)
            des_d = os.path.join(aic_root, 'application', app_os, new_app)
            src_d = os.path.normpath(src_d)
            des_d = os.path.normpath(des_d)
            if not os.path.exists(des_d):
                print("Copy {} to {}".format(src_d, des_d))
                if platform.system() == 'Linux':
                    shutil.copytree(src_d, des_d)
                elif platform.system() == 'Windows':
                    shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)
        print("Add board {} for {} succeed!".format(new_board, manuf_name))
        exit(0)


# cmd-option: aicupg
def aicupg_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--aicupg', dest='aicupg', action='store_true',
              default=False, help='Call upgcmd to burn image')

    aicupg = GetOption('aicupg')
    if not aicupg:
        return

    if platform.system() == 'Windows':
        upgcmd = os.path.join(aic_root, 'tools/scripts/upgcmd.exe')
    else:
        upgcmd = os.path.join(aic_root, 'tools/scripts/upgcmd')
    if not os.path.exists(upgcmd):
        print('{} does not exist!'.format(upgcmd))
        sys.exit(100)

    solution = get_prj_defconfig(aic_root).replace('_defconfig', '')
    img_path = os.path.join(aic_root, 'output/' + solution + '/images')
    if not os.path.exists(img_path):
        print('{} does not exist! Need build the project first.'.format(img_path))
        sys.exit(100)
    for file in sorted(os.listdir(img_path)):
        if file.endswith('.img'):
            img_file = file
            break

    print('Try to burn image file: {} ...'.format(img_file))
    os.system(upgcmd + ' image ' + os.path.join(img_path, img_file))
    os.system(upgcmd + ' shcmd reset ')
    sys.exit(0)


def get_file_md5(file):
    MD5 = hashlib.new('md5')
    with open(file, 'r') as fp:
        MD5.update(fp.read().encode('utf8'))
        fp_md5 = MD5.hexdigest()
        return fp_md5


# cmd-option: windows menuconfig
def win_menuconfig_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    # windows run menuconfig cmd
    if platform.system() == 'Windows':
        AddOption('--menuconfig', dest='win_menuconfig', action='store_true',
                  default=False, help='make menuconfig for luban-lite')
        win_menuconfig = GetOption('win_menuconfig')
        if win_menuconfig:
            md5dot0 = get_file_md5('.config')
            os.system('menuconfig')
            md5dot1 = get_file_md5('.config')
            changed = not operator.eq(md5dot0, md5dot1)
            if changed:
                save_defconfig(aic_root)
            # If 'output/xxxx/project_eclipse' folder exists need update eclipse project
            prj_name = prj_defconfig.replace('_defconfig', '')
            prj_eclipse_dir = os.path.join(aic_root, 'output', prj_name, 'project_eclipse')
            if not os.path.exists(prj_eclipse_dir):
                exit(0)


# check qemu tool
def chk_qemu_tool(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    if platform.system() == 'Linux':
        qemu_name = 'qemu-linux'
        qemu_tar_name = 'xuantie-qemu-x86_64'
    elif platform.system() == 'Windows':
        qemu_name = 'qemu-win'
        qemu_tar_name = 'xuantie-qemu-win'
    qemu_ppath = os.path.join(aic_root, 'tools/env/tools/qemu/')
    qemu_path = os.path.join(qemu_ppath, qemu_name)
    qemu_path = os.path.normpath(qemu_path)
    # check
    if os.path.exists(qemu_path):
        return
    # prepare
    for root, dirs, files in os.walk(qemu_ppath):
        if root != qemu_ppath:
            break
        for f in files:
            if f.startswith(qemu_tar_name) and f.find('.tar') >= 0:
                abs_f = os.path.join(qemu_ppath, f)
                os.mkdir(qemu_path)

                pr_info('Extract ' + qemu_tar_name + ' ...')
                untar(abs_f, qemu_path, 0)
                break


# cmd-option: run qemu
def run_qemu_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--run-qemu', dest='run_qemu', action='store_true',
              default=False, help='use qemu run target')
    run_qemu = GetOption('run_qemu')
    if run_qemu:
        # check qemu tool
        chk_qemu_tool(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig)
        # qemu
        chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
        sys.path.append(chip_path)
        import rtconfig
        if rtconfig.DEVICE.find('rv32') >= 0:
            qemu_name = 'qemu-system-riscv32'
            machine = 'smartl'
        else:
            qemu_name = 'qemu-system-riscv64'
            machine = 'smarth'
        if platform.system() == 'Linux':
            qemu_path = os.path.join(aic_root, 'tools/env/tools/qemu/qemu-linux/bin', qemu_name)
        elif platform.system() == 'Windows':
            qemu_path = os.path.join(aic_root, 'tools/env/tools/qemu/qemu-win', qemu_name)
        # target
        prj_name = prj_defconfig.replace('_defconfig', '')
        prj_out_dir = 'output/' + prj_name + '/images/'
        target = os.path.join(aic_root, prj_out_dir, rtconfig.SOC + '.' + rtconfig.TARGET_EXT)
        # complete cmd
        cmd = qemu_path + ' -cpu ' + rtconfig.CPUNAME + ' -M ' + machine + \
            ' -kernel ' + target + ' -nographic'
        os.system(cmd)
        exit(0)


# cmd-option: list size
def list_size_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--list-size', dest='list_size', action='store_true',
              default=False, help='list all obj file size')
    list_size = GetOption('list_size')
    if list_size:
        # size
        gcc_size = os.path.join(aic_root, 'toolchain/bin/riscv64-unknown-elf-size')
        # obj foler
        prj_name = prj_defconfig.replace('_defconfig', '')
        prj_out_dir = os.path.join(aic_root, 'output/' + prj_name)
        # target
        chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
        sys.path.append(chip_path)
        import rtconfig
        target = os.path.join(prj_out_dir, 'images', rtconfig.SOC + '.' + rtconfig.TARGET_EXT)
        # cmd
        p = []
        for root, dirs, files in os.walk(prj_out_dir):
            if root.endswith('images'):
                continue
            for file in files:
                path = os.path.join(root, file)
                p.append(path)
        p.sort()
        paths = target + ' ' + ' '.join(p)
        cmd = gcc_size + ' ' + paths
        os.system(cmd)
        exit(0)


g_mem_items = {
  ('d21x'):
  [
    # 'regions_level0':
    [
      [['dram', 0], ['__dram_start', '__dram_end'], [0x0, 0x0]],
    ],
    # 'regions_level1':
    [
      [['dram_sw', 0], ['__dram_start', '__dram_sw_end'], [0x0, 0x0]],
      [['dram_cma', 0], ['__dram_sw_end', '__dram_cma_end'], [0x0, 0x0]],
    ],
    # 'regions_level2':
    [
      [['dram_sw_static', 0], ['__dram_start', '__dram_sw_heap_start'], [0x0, 0x0]],
      [['dram_sw_heap', 0], ['__dram_sw_heap_start', '__dram_sw_heap_end'], [0x0, 0x0]],
      [['dram_cma_static', 0], ['__dram_cma_start', '__dram_cma_heap_start'], [0x0, 0x0]],
      [['dram_cma_heap', 0], ['__dram_cma_heap_start', '__dram_cma_heap_end'], [0x0, 0x0]],
    ],
    # 'regions_level3':
    [
      [['.text', 0], ['__stext', '__etext'], [0x0, 0x0]],
      [['.rodata', 0], ['__srodata', '__erodata'], [0x0, 0x0]],
      [['.data', 0], ['__sdata', '__edata'], [0x0, 0x0]],
      [['.bss', 0], ['__sbss', '__ebss'], [0x0, 0x0]],
      [['.heap_sys', 0], ['__heap_start', '__heap_end'], [0x0, 0x0]],
      [['.heap_cma', 0], ['__cma_heap_start', '__cma_heap_end'], [0x0, 0x0]],
    ],
  ],

  ('d13x'):
  [
    # 'regions_level0':
    [
      [['itcm', 0], ['__itcm_start', '__itcm_end'], [0x0, 0x0]],
      [['dtcm', 0], ['__dtcm_start', '__dtcm_end'], [0x0, 0x0]],
      [['sram_s0', 0], ['__sram_s0_start', '__sram_s0_end'], [0x0, 0x0]],
      [['sram_s1', 0], ['__sram_s1_start', '__sram_s1_end'], [0x0, 0x0]],
      [['psram', 0], ['__psram_start', '__psram_end'], [0x0, 0x0]],
      [['xip', 0], ['__xip_start', '__xip_end'], [0x0, 0x0]]
    ],
    # 'regions_level1':
    [
      [['itcm_code', 0], ['__itcm_code_start', '__itcm_code_end'], [0x0, 0x0]],
      [['itcm_heap', 0], ['__itcm_heap_start', '__itcm_heap_end'], [0x0, 0x0]],
      [['dtcm_data', 0], ['__dtcm_data_start', '__dtcm_data_end'], [0x0, 0x0]],
      [['dtcm_heap', 0], ['__dtcm_heap_start', '__dtcm_heap_end'], [0x0, 0x0]],
      [['sram_s0_static', 0], ['__sram_s0_start', '__sram_s0_sw_heap_start'], [0x0, 0x0]],
      [['sram_s0_heap', 0], ['__sram_s0_sw_heap_start', '__sram_s0_sw_heap_end'], [0x0, 0x0]],
      [['sram_s1_sw', 0], ['__sram_s1_sw_start', '__sram_s1_sw_end'], [0x0, 0x0]],
      [['sram_s1_cma', 0], ['__sram_s1_cma_start', '__sram_s1_cma_end'], [0x0, 0x0]],
      [['psram_sw', 0], ['__psram_sw_start', '__psram_sw_end'], [0x0, 0x0]],
      [['psram_cma', 0], ['__psram_cma_start', '__psram_cma_end'], [0x0, 0x0]],
    ],
    # 'regions_level2':
    [
      [['sram_s1_sw_static', 0], ['__sram_s1_sw_start', '__sram_s1_sw_heap_start'], [0x0, 0x0]],
      [['sram_s1_sw_heap', 0], ['__sram_s1_sw_heap_start', '__sram_s1_sw_heap_end'], [0x0, 0x0]],
      [['sram_s1_cma_static', 0], ['__sram_s1_cma_start', '__sram_s1_cma_heap_start'], [0x0, 0x0]],
      [['sram_s1_cma_heap', 0], ['__sram_s1_cma_heap_start', '__sram_s1_cma_heap_end'], [0x0, 0x0]],
      [['psram_sw_static', 0], ['__psram_sw_start', '__psram_sw_heap_start'], [0x0, 0x0]],
      [['psram_sw_heap', 0], ['__psram_sw_heap_start', '__psram_sw_heap_end'], [0x0, 0x0]],
      [['psram_cma_static', 0], ['__psram_cma_start', '__psram_cma_heap_start'], [0x0, 0x0]],
      [['psram_cma_heap', 0], ['__psram_cma_heap_start', '__psram_cma_heap_end'], [0x0, 0x0]],
    ],
    # 'regions_level3':
    [
      [['.text', 0], ['__stext', '__etext'], [0x0, 0x0]],
      [['.rodata', 0], ['__srodata', '__erodata'], [0x0, 0x0]],
      [['.data', 0], ['__sdata', '__edata'], [0x0, 0x0]],
      [['.bss', 0], ['__sbss', '__ebss'], [0x0, 0x0]],
      [['.heap_sys', 0], ['__heap_start', '__heap_end'], [0x0, 0x0]],
      [['.heap_cma', 0], ['__cma_heap_start', '__cma_heap_end'], [0x0, 0x0]],
    ],
  ],

  ('g73x'):
  [
    # 'regions_level0':
    [
      [['itcm', 0], ['__itcm_start', '__itcm_end'], [0x0, 0x0]],
      [['dtcm', 0], ['__dtcm_start', '__dtcm_end'], [0x0, 0x0]],
      [['sram_s0', 0], ['__sram_s0_start', '__sram_s0_end'], [0x0, 0x0]],
      [['sram_s1', 0], ['__sram_s1_start', '__sram_s1_end'], [0x0, 0x0]],
      [['psram', 0], ['__psram_start', '__psram_end'], [0x0, 0x0]],
      [['xip', 0], ['__xip_start', '__xip_end'], [0x0, 0x0]]
    ],
    # 'regions_level1':
    [
      [['itcm_code', 0], ['__itcm_code_start', '__itcm_code_end'], [0x0, 0x0]],
      [['itcm_heap', 0], ['__itcm_heap_start', '__itcm_heap_end'], [0x0, 0x0]],
      [['dtcm_data', 0], ['__dtcm_data_start', '__dtcm_data_end'], [0x0, 0x0]],
      [['dtcm_heap', 0], ['__dtcm_heap_start', '__dtcm_heap_end'], [0x0, 0x0]],
      [['sram_s0_static', 0], ['__sram_s0_start', '__sram_s0_sw_heap_start'], [0x0, 0x0]],
      [['sram_s0_heap', 0], ['__sram_s0_sw_heap_start', '__sram_s0_sw_heap_end'], [0x0, 0x0]],
      [['sram_s1_sw', 0], ['__sram_s1_sw_start', '__sram_s1_sw_end'], [0x0, 0x0]],
      [['sram_s1_cma', 0], ['__sram_s1_cma_start', '__sram_s1_cma_end'], [0x0, 0x0]],
      [['psram_sw', 0], ['__psram_sw_start', '__psram_sw_end'], [0x0, 0x0]],
      [['psram_cma', 0], ['__psram_cma_start', '__psram_cma_end'], [0x0, 0x0]],
    ],
    # 'regions_level2':
    [
      [['sram_s1_sw_static', 0], ['__sram_s1_sw_start', '__sram_s1_sw_heap_start'], [0x0, 0x0]],
      [['sram_s1_sw_heap', 0], ['__sram_s1_sw_heap_start', '__sram_s1_sw_heap_end'], [0x0, 0x0]],
      [['sram_s1_cma_static', 0], ['__sram_s1_cma_start', '__sram_s1_cma_heap_start'], [0x0, 0x0]],
      [['sram_s1_cma_heap', 0], ['__sram_s1_cma_heap_start', '__sram_s1_cma_heap_end'], [0x0, 0x0]],
      [['psram_sw_static', 0], ['__psram_sw_start', '__psram_sw_heap_start'], [0x0, 0x0]],
      [['psram_sw_heap', 0], ['__psram_sw_heap_start', '__psram_sw_heap_end'], [0x0, 0x0]],
      [['psram_cma_static', 0], ['__psram_cma_start', '__psram_cma_heap_start'], [0x0, 0x0]],
      [['psram_cma_heap', 0], ['__psram_cma_heap_start', '__psram_cma_heap_end'], [0x0, 0x0]],
    ],
    # 'regions_level3':
    [
      [['.text', 0], ['__stext', '__etext'], [0x0, 0x0]],
      [['.rodata', 0], ['__srodata', '__erodata'], [0x0, 0x0]],
      [['.data', 0], ['__sdata', '__edata'], [0x0, 0x0]],
      [['.bss', 0], ['__sbss', '__ebss'], [0x0, 0x0]],
      [['.heap_sys', 0], ['__heap_start', '__heap_end'], [0x0, 0x0]],
      [['.heap_cma', 0], ['__cma_heap_start', '__cma_heap_end'], [0x0, 0x0]],
    ],
  ],

  ('d12x'):
  [
    # 'regions_level0':
    [
      [['sram', 0], ['__sram_start', '__sram_end'], [0x0, 0x0]],
      [['psram', 0], ['__psram_start', '__psram_end'], [0x0, 0x0]],
      [['xip', 0], ['__xip_start', '__xip_end'], [0x0, 0x0]]
    ],
    # 'regions_level1':
    [
      [['sram_sw', 0], ['__sram_sw_start', '__sram_sw_end'], [0x0, 0x0]],
      [['sram_cma', 0], ['__sram_cma_start', '__sram_cma_end'], [0x0, 0x0]],
      [['psram_sw', 0], ['__psram_sw_start', '__psram_sw_end'], [0x0, 0x0]],
      [['psram_cma', 0], ['__psram_cma_start', '__psram_cma_end'], [0x0, 0x0]],
    ],
    # 'regions_level2':
    [
      [['sram_sw_static', 0], ['__sram_sw_start', '__sram_sw_heap_start'], [0x0, 0x0]],
      [['sram_sw_heap', 0], ['__sram_sw_heap_start', '__sram_sw_heap_end'], [0x0, 0x0]],
      [['sram_cma_static', 0], ['__sram_cma_start', '__sram_cma_heap_start'], [0x0, 0x0]],
      [['sram_cma_heap', 0], ['__sram_cma_heap_start', '__sram_cma_heap_end'], [0x0, 0x0]],
      [['psram_sw_static', 0], ['__psram_sw_start', '__psram_sw_heap_start'], [0x0, 0x0]],
      [['psram_sw_heap', 0], ['__psram_sw_heap_start', '__psram_sw_heap_end'], [0x0, 0x0]],
      [['psram_cma_static', 0], ['__psram_cma_start', '__psram_cma_heap_start'], [0x0, 0x0]],
      [['psram_cma_heap', 0], ['__psram_cma_heap_start', '__psram_cma_heap_end'], [0x0, 0x0]],
    ],
    # 'regions_level3':
    [
      [['.text', 0], ['__stext', '__etext'], [0x0, 0x0]],
      [['.rodata', 0], ['__srodata', '__erodata'], [0x0, 0x0]],
      [['.data', 0], ['__sdata', '__edata'], [0x0, 0x0]],
      [['.bss', 0], ['__sbss', '__ebss'], [0x0, 0x0]],
      [['.heap_sys', 0], ['__heap_start', '__heap_end'], [0x0, 0x0]],
      [['.heap_cma', 0], ['__cma_heap_start', '__cma_heap_end'], [0x0, 0x0]],
    ],
  ],
}


def get_mem_item_addr(sym_table, item):
    ret = -2
    s = 0
    e = 0
    for i in range(0, 2):
        addr_list = re.findall('([0-9a-fA-F]{8,16}).*' + item[i], sym_table)
        if addr_list == []:
            continue
        addr = int(addr_list[0], 16)
        ret += 1
        if i == 0:
            s = addr
        else:
            e = addr
    if ret == 0 and s == e:
        ret = -100
    # print('{}: {:#x} - {:#x}, ret = {}'.format(item, s, e, ret))
    return ret, s, e


def print_mem_item(chip_mem_items, prefix, lvl, p_s, p_e):
    max_lvl = len(chip_mem_items)
    if lvl >= max_lvl:
        return
    if lvl == 0:
        print('{:^23}{}{:^10} {:^10}  {:^10}'.format('Region', ' ' * ((max_lvl - lvl) * 5),
              'Start', 'End', 'Length'))
    found = 0
    for item in chip_mem_items[lvl]:
        ret = item[0][1]
        name = item[0][0]
        s = item[2][0]
        e = item[2][1]
        if (ret != 0) or (s < p_s) or (e > p_e):
            continue
        found = 1
        sys.stdout.write(prefix)
        if lvl != 0:
            sys.stdout.write('  └─ ')
        print('{:20}{}0x{:0>8x} - 0x{:0>8x}  0x{:0>8x}'.format(name, ' ' * ((max_lvl - lvl) * 5),
              s, e, e-s))
        if lvl == 0:
            print_mem_item(chip_mem_items, prefix, lvl+1, s, e)
        else:
            print_mem_item(chip_mem_items, prefix + '     ', lvl+1, s, e)
    if found == 0:
        print_mem_item(chip_mem_items, prefix + '     ', lvl+1, p_s, p_e)


# cmd-option: list mem
def list_mem_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--list-mem', dest='list_mem', action='store_true',
              default=False, help='list all memory layout')
    list_mem = GetOption('list_mem')
    if list_mem:
        # obj foler
        prj_name = prj_defconfig.replace('_defconfig', '')
        prj_out_dir = os.path.join(aic_root, 'output/' + prj_name)
        # target
        chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
        sys.path.append(chip_path)
        import rtconfig
        target = os.path.join(prj_out_dir, 'images', rtconfig.SOC + '.' + rtconfig.TARGET_EXT)
        if not os.path.exists(target):
            print('{} does not exist! Need build the project first.'.format(target))
            sys.exit(100)
        # get symbol table
        bin_nm = os.path.join(aic_root, 'toolchain/bin/' + rtconfig.PREFIX + 'nm')
        cmd = bin_nm + ' ' + target
        sym_table = os.popen(cmd).read()
        chip_mem_items = []
        for chip_set in g_mem_items.keys():
            if prj_chip in chip_set:
                chip_mem_items = g_mem_items[chip_set]
        if chip_mem_items == []:
            print('{} definition not found in g_mem_items[]!'.format(prj_chip))
            sys.exit(200)
        # get symbol addr
        for key in range(len(chip_mem_items)):
            for item in chip_mem_items[key]:
                ret, s, e = get_mem_item_addr(sym_table, item[1])
                item[0][1] = ret
                item[2][0] = s
                item[2][1] = e
        # print mem layout
        print('{} Memory layout:\n'.format(os.path.relpath(target, aic_root)))
        print_mem_item(chip_mem_items, '', 0, 0, 0xFFFFFFFF)
        exit(0)


# cmd-option: list module
def list_module_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--list-module', dest='list_module', action='store_true',
              default=False, help='list all opened module info')
    list_module = GetOption('list_module')
    if list_module:
        prj_out_dir = os.path.join(aic_root, '.config')
        prj_name = prj_defconfig.replace('_defconfig', '')
        print('Load modules information from ' + prj_name)

        list_tit = ["Module", "Version", "Device"]
        list_item = []

        with open(prj_out_dir, 'r') as file:
            # get opened modules name and version
            pattern = 'CONFIG_AIC_.*_DRV_V.*=y'
            for f in file:
                match = re.search(pattern, f)
                if match:
                    matched_str = f.split('_DRV_V')
                    mod = matched_str[0].split('CONFIG_AIC_')[1]
                    ver = matched_str[1].split('=y\n')[0]
                    version = 'V' + ver[0] + '.' + ver[1]
                    list_item.append([mod, version])

            file.seek(0)
            # get opened device
            pattern = r'CONFIG_AIC_USING_[^_]+\d+=y'
            list_items_dev = [[] for _ in range(len(list_item))]
            items_mod = [x[0] for x in list_item]

            for f in file:
                match = re.search(pattern, f)
                if not match:
                    continue
                dev_str = f.split('CONFIG_AIC_USING_')[1].split('=y\n')[0]
                for i in range(len(items_mod)):
                    match = re.search(items_mod[i], dev_str)
                    if not match:
                        continue
                    list_items_dev[i].append(dev_str)

            for i, item in enumerate(list_items_dev):
                list_item[i] += (item,)

            # Sort in ascending order based on the first value of each element
            sorted_list_items = sorted(list_item, key=lambda x: x[0])
            file.close()

        if not sorted_list_items:
            pr_err('The module list is empty')
            exit(110)

        items_mod = [x[0] for x in sorted_list_items]
        items_ver = [x[1] for x in sorted_list_items]
        items_dev = [x[2] for x in sorted_list_items]

        len_mod = max(len(s) for s in items_mod)
        len_ver = max(len(s) for s in items_ver)
        # step: Interval between each title
        step = 1
        len_total = 0
        min_len_item = max(len(s) for s in list_tit)
        max_len_dev_strs = 0
        len_dev_strs = 0
        num_dev = 0

        # get the length of the longest bit array in a two-dimensional array
        # and the longest element of the longest bit array
        for i in range(len(items_dev)):
            num_dev = len(items_dev[i])
            for dev_str in items_dev[i]:
                len_dev_strs += len(dev_str) + step
            if (len_dev_strs > max_len_dev_strs):
                max_len_dev_strs = len_dev_strs
                max_num_dev = num_dev
            len_dev_strs = 0

        len_dev = max_len_dev_strs - step
        list_len = [len_mod, len_ver, len_dev]
        num_tit = len(list_len)

        for i in range(num_tit):
            if list_len[i] < min_len_item:
                list_len[i] = min_len_item
            len_total += list_len[i]

        print("=" * (len_total + (num_tit - 1) * step))
        for i in range(num_tit):
            print("{:<{}}".format(list_tit[i], list_len[i])),

        print("")
        for i in range(num_tit):
            print("-" * list_len[i]),

        for i in range(len(items_mod)):
            print("")
            print("{:<{}}{:<{}}".format(items_mod[i], list_len[0] + step,
                                        items_ver[i], list_len[1])),
            for item in items_dev[i]:
                if len(item):
                    print("" + item),
        print("")
        print("=" * (len_total + (num_tit - 1) * step))
        exit(0)


# cmd-option: distclean
def distclean_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--distclean', dest='distclean', action='store_true',
              default=False, help='clean the toolchain and the output folder')
    distclean = GetOption('distclean')
    if distclean:
        prj_name = prj_defconfig.replace('_defconfig', '')
        os.system('rm -rf ' + aic_root + '/output/' + prj_name)
        os.system('rm -rf ' + aic_root + '/toolchain')
        os.system('rm -f ' + aic_root + '/.config')
        exit(0)


# check & prepare toolchain
def chk_prepare_toolchain(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    config_file = os.path.join(aic_root, '.config')
    if get_config(config_file, 'CONFIG_ARCH_CSKY'):
        if platform.system() == 'Linux':
            toolchain_name = 'csky-elf-noneabiv2-tools-x86_64-newlib'
        elif platform.system() == 'Windows':
            toolchain_name = 'csky-elf-noneabiv2-tools-mingw-newlib'
    else:
        if platform.system() == 'Linux':
            toolchain_name = 'Xuantie-900-gcc-elf-newlib-x86_64'
        elif platform.system() == 'Windows':
            toolchain_name = 'Xuantie-900-gcc-elf-newlib-mingw'

        toolchain_ver = get_config(config_file, 'CONFIG_PRJ_TOOLCHAIN_VER')
        if toolchain_ver:
            toolchain_name += '-' + toolchain_ver.replace('"', '')
        else:
            toolchain_name += '-' + 'V2.6.1'

    os.chdir(aic_root)
    toolchain_ppath = 'tools/toolchain/'
    toolchain_bpath = 'toolchain'
    # check
    ready_f = os.path.join(toolchain_bpath, '.ready')
    if os.path.exists(ready_f):
        with open(ready_f, 'r') as f:
            toolchain_os = f.readline().strip()
            old_toolchain_name = f.readline().strip()
            f.close()
            if platform.system() != toolchain_os:
                shutil.rmtree(toolchain_bpath)
            else:
                if old_toolchain_name != toolchain_name:
                    print('Switch toolchain: {} -> {}'.format(old_toolchain_name, toolchain_name))
                    shutil.rmtree(toolchain_bpath)
                else:
                    return
    # prepare
    trim_tar = ''
    full_tar = ''
    for root, dirs, files in os.walk(toolchain_ppath):
        if root != toolchain_ppath:
            break
        for f in files:
            if f.startswith(toolchain_name) and f.endswith('-trim.tar.gz'):
                trim_tar = f
            elif f.startswith(toolchain_name) and f.endswith('.tar.gz'):
                full_tar = f
            else:
                continue
    if trim_tar != '':
        tar_f = trim_tar
    elif full_tar != '':
        tar_f = full_tar
    else:
        print("Coundn't find toolchain tar file.")
        return
    abs_f = os.path.join(toolchain_ppath, tar_f)
    pr_info('Extract toolchain ' + tar_f + ' ...')
    if not os.path.exists(toolchain_bpath):
        os.mkdir(toolchain_bpath)
    untar(abs_f, toolchain_bpath, 1)

    with open(ready_f, 'w') as f:
        f.write(platform.system() + "\n")
        f.write(toolchain_name)


def trim_folder(folders, prefix, saves):
    for folder in folders:
        if not os.path.exists(folder):
            continue
        for root, dirs, files in os.walk(folder):
            if root != folder:
                break
            for d in dirs:
                if d.startswith(prefix) and d not in saves:
                    shutil.rmtree(os.path.join(folder, d))


def trim_toolchain(aic_root, tar_path, exec_path, tar_prefix, cross_prefix, marchs):
    toolchain_ppath = os.path.join(aic_root, tar_path)
    toolchain_bpath = os.path.join(aic_root, exec_path)
    toolchain_prefix = tar_prefix
    toolchain_suffix = '.tar.gz'
    toolchain_trim = '-trim'
    toolchain_cross = cross_prefix
    for root, dirs, files in os.walk(toolchain_ppath):
        if root != toolchain_ppath:
            break
        for f in files:
            if f.startswith(toolchain_prefix) and f.endswith(toolchain_suffix) and \
               f.find(toolchain_trim) < 0:
                abs_f = os.path.join(toolchain_ppath, f)
                toolchain_name = re.findall('(.+?)\\.tar\\.gz', f)[0]
                toolchain_xpath = os.path.join(toolchain_ppath, toolchain_name)
                if os.path.exists(toolchain_xpath):
                    shutil.rmtree(toolchain_xpath)
                pr_info('Extract toolchain ' + f + ' ...')
                if not os.path.exists(toolchain_xpath):
                    os.mkdir(toolchain_xpath)
                untar(abs_f, toolchain_xpath, 1)
                pr_info('Trim toolchain ' + f + ' ...')
                folder1 = os.path.join(toolchain_xpath, toolchain_cross + '/lib')
                p_folder2 = os.path.join(toolchain_xpath, 'lib/gcc/' + toolchain_cross)
                folder2 = glob.glob(p_folder2 + '/*')[0]
                folders = [folder1, folder2]
                trim_folder(folders, 'rv', marchs)
                toolchain_trimname = toolchain_name + toolchain_trim + toolchain_suffix
                pr_info('Repack trim toolchain ' + toolchain_trimname + ' ...')
                abs_f = os.path.join(toolchain_ppath, toolchain_trimname)
                if os.path.exists(abs_f):
                    os.remove(abs_f)
                tar(abs_f, toolchain_ppath, toolchain_name)
                shutil.rmtree(toolchain_xpath)
    if os.path.exists(toolchain_bpath):
        shutil.rmtree(toolchain_bpath)


# cmd-option: trim-toolchain
def trimtoolchain_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--trim-toolchain', dest='trim-toolchain', action='store_true',
              default=False, help='trim toolchain, only retian the arch used by SDK.')
    trimtoolchain = GetOption('trim-toolchain')
    if trimtoolchain:
        # collect riscv arch in SDK
        path = os.path.join(aic_root, 'target')
        chips = get_all_chip(path)
        marchs = []
        m_marchs = []
        for chip in chips:
            chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + chip)
            sys.path.append(chip_path)
            import rtconfig
            reload(rtconfig)
            if hasattr(rtconfig, 'TOOLCHAIN_LIB'):
                marchs.append(rtconfig.TOOLCHAIN_LIB)
            if hasattr(rtconfig, 'M_TOOLCHAIN_LIB'):
                m_marchs.append(rtconfig.M_TOOLCHAIN_LIB)
            sys.path.remove(chip_path)
        marchs = list(set(marchs))
        m_marchs = list(set(m_marchs))
        print('marchs = {}. \nm_marchs = {}'.format(marchs, m_marchs))

        # trim kernel toolchain
        trim_toolchain(aic_root, 'tools/toolchain/', 'toolchain', 'Xuantie-900-gcc-elf-newlib',
                       'riscv64-unknown-elf', marchs)

        # trim dm-app toolchain
        trim_toolchain(aic_root, 'packages/artinchip/aic-dm-apps/tools/toolchain',
                       'packages/artinchip/aic-dm-apps/toolchain', 'xpack-riscv-none-embed-gcc',
                       'riscv-none-embed', m_marchs)
        exit(0)


def mkimage_get_mtdpart_size(imgname):
    partlist = prj_out_dir + 'partition_file_list.h'
    size = 0
    if not os.path.exists(partlist):
        return 0
    with open(partlist) as f:
        lines = f.readlines()
        for ln in lines:
            name = ln.split(',')[1].replace('"', '').replace('*', '')
            if imgname == re.sub(".sparse", "", name):
                size = int(ln.split(',')[2])
                return size
    print('Image {} is not used in any partition'.format(imgname))
    print('please check your project\'s image_cfg.json')
    return size


def mkimage_get_nand_params():
    partlist = prj_out_dir + 'partition_file_list.h'
    if not os.path.exists(partlist):
        return None
    nands = ""
    with open(partlist) as f:
        lines = f.readlines()
        for ln in lines:
            if "nands=" in ln:
                nands = ln[6:]
                break
    if len(nands) == 0:
        return None
    nands_list = nands.strip().split(';')
    ret_list = []
    for nandparam in nands_list:
        ret_list.append(tuple(nandparam.split(',')))
    return ret_list


def mkimage_gen_uffs_script(srcdir, script):
    fs = open(script, 'w+')
    root_path = srcdir
    for root, dirs, files in os.walk(srcdir):
        for sub in dirs:
            dirpath = os.path.join(root, sub).replace(root_path, '').replace('\\', '/')
            if not dirpath.startswith('/'):
                dirpath = '/' + dirpath
            fs.write('mkdir {}\n'.format(dirpath))
        for fn in files:
            fpath = os.path.join(root, fn)
            srcpath = '::' + fpath
            dstpath = fpath.replace(root_path, '').replace('\\', '/')
            if not dstpath.startswith('/'):
                dstpath = '/' + dstpath
            fs.write('cp {} {}\n'.format(srcpath, dstpath))
    fs.close()


def mkimage_check_file_exist(pathdir, filename):
    restr = filename.replace('.', '\\.').replace('*', '.*')
    ents = os.listdir(pathdir)
    for ent in ents:
        matchobj = re.match(restr, ent)
        if matchobj:
            return True
    return False


def mkimage_gen_empty_img(outimg, size):
    f = open(outimg, 'wb')
    f.write(bytearray(size))
    f.close()


def mkimage_gen_mkfs_action(img_id):
    img_enable = 'CONFIG_AIC_USING_FS_IMAGE_{}'.format(img_id)
    if get_config(prj_root_dir + ".config", img_enable) != 'y':
        return ''
    fatfs_enable = 'CONFIG_AIC_USING_FS_IMAGE_TYPE_FATFS_FOR_{}'.format(img_id)
    littlefs_enable = 'CONFIG_AIC_USING_FS_IMAGE_TYPE_LITTLEFS_FOR_{}'.format(img_id)
    uffs_enable = 'CONFIG_AIC_USING_FS_IMAGE_TYPE_UFFS_FOR_{}'.format(img_id)
    mkfscmd = ''
    if get_config(prj_root_dir + '.config', fatfs_enable) == 'y':
        imgname_opt = 'CONFIG_AIC_FS_IMAGE_NAME_{}'.format(img_id)
        imgname = get_config(prj_root_dir + '.config', imgname_opt).replace('"', '')
        auto_opt = 'CONFIG_AIC_FATFS_AUTO_SIZE_FOR_{}'.format(img_id)
        volab_opt = 'CONFIG_AIC_FATFS_DEFAULT_VOLAB_{}'.format(img_id)
        srcdir_opt = 'CONFIG_AIC_FS_IMAGE_DIR_{}'.format(img_id)
        srcdir = get_config(prj_root_dir + '.config', srcdir_opt).replace('"', '')
        os.environ["aic_fs_image_dir"] = srcdir
        srcdir = srcdir
        if srcdir.endswith('/'):
            srcdir = srcdir[0:len(srcdir) - 1]
        outimg = prj_out_dir + imgname
        data_siz = 0

        # when CONFIG_AIC_FATFS_AUTO_SIZE_FOR_0 is not set, should be check None
        if get_config(prj_root_dir + '.config', auto_opt) is not None:
            auto_siz = get_config(prj_root_dir + '.config', auto_opt).replace('"', '')
        else:
            auto_siz = 'n'
        # when CONFIG_AIC_FATFS_DEFAULT_LABEL_0 is not set, should be check None
        if get_config(prj_root_dir + '.config', volab_opt) is not None:
            volab = 'default'
        else:
            volab = 'CONFIG_AIC_FATFS_VOLAB_{}'.format(img_id)
            volab = get_config(prj_root_dir + '.config', volab)

        cluster_size = 'CONFIG_AIC_USING_FS_IMAGE_TYPE_FATFS_CLUSTER_SIZE'
        cluster = int(get_config(prj_root_dir + '.config', cluster_size))

        cmdstr = 'python3 ' + aic_script_dir + 'makefatfs.py '
        if auto_siz == 'y':
            sector_siz = 512
            # use full part size
            # cmdstr += '--auto '
            cmdstr += '--fullpart '
        else:
            sector_opt = 'CONFIG_AIC_FATFS_SECTOR_SIZE_FOR_{}'.format(img_id)
            sector_siz = int(get_config(prj_root_dir + '.config', sector_opt))
            sectcnt_opt = 'CONFIG_AIC_FATFS_SECTOR_COUNT_FOR_{}'.format(img_id)
            sector_cnt = int(get_config(prj_root_dir + '.config', sectcnt_opt))
            data_siz = sector_siz * sector_cnt
            cmdstr += '--imgsize {} '.format(data_siz)

        cmdstr += '--volab {} '.format(volab)
        cmdstr += '--cluster {} '.format(cluster)
        cmdstr += '--sector {} '.format(sector_siz)
        cmdstr += '--tooldir {} '.format(aic_script_dir)
        cmdstr += '--inputdir {} '.format(srcdir)
        cmdstr += '--outfile {}\n'.format(outimg)
        mkfscmd += cmdstr
    if get_config(prj_root_dir + '.config', littlefs_enable) == 'y':
        imgname_opt = 'CONFIG_AIC_FS_IMAGE_NAME_{}'.format(img_id)
        imgname = get_config(prj_root_dir + '.config', imgname_opt).replace('"', '')
        blksiz_opt = 'CONFIG_AIC_LITTLEFS_BLOCK_SIZE_FOR_{}'.format(img_id)
        blksiz = get_config(prj_root_dir + '.config', blksiz_opt)
        pagesiz_opt = 'CONFIG_AIC_LITTLEFS_PAGE_SIZE_FOR_{}'.format(img_id)
        pagesiz = get_config(prj_root_dir + '.config', pagesiz_opt)
        srcdir_opt = 'CONFIG_AIC_FS_IMAGE_DIR_{}'.format(img_id)
        srcdir = get_config(prj_root_dir + '.config', srcdir_opt).replace('"', '')
        os.environ["aic_fs_image_dir"] = srcdir
        srcdir = srcdir
        outimg = prj_out_dir + imgname
        cmdstr = 'python3 ' + aic_script_dir + 'makelittlefs.py '
        cmdstr += '--pagesize {} '.format(pagesiz)
        cmdstr += '--blocksize {} '.format(blksiz)
        cmdstr += '--tooldir {} '.format(aic_script_dir)
        cmdstr += '--inputdir {} '.format(srcdir)
        cmdstr += '--outfile {}\n'.format(outimg)
        mkfscmd += cmdstr
    if get_config(prj_root_dir + '.config', uffs_enable) == 'y':
        nand_list = mkimage_get_nand_params()
        if nand_list is None:
            return ''
        imgname_opt = 'CONFIG_AIC_FS_IMAGE_NAME_{}'.format(img_id)
        imgname = get_config(prj_root_dir + '.config', imgname_opt).replace('"', '')
        imgsize = mkimage_get_mtdpart_size(imgname)
        srcdir_opt = 'CONFIG_AIC_FS_IMAGE_DIR_{}'.format(img_id)
        srcdir = get_config(prj_root_dir + '.config', srcdir_opt).replace('"', '')
        os.environ["aic_fs_image_dir"] = srcdir
        srcdir = srcdir
        for param in nand_list:
            (pagesiz, blksiz, oobsiz) = param
            pagesiz = int(pagesiz)
            blksiz = int(blksiz)
            oobsiz = int(oobsiz)
            blkpagecnt = int(blksiz / pagesiz)
            partblkcnt = int(imgsize / blksiz)
            prefix = 'page_{}k_block_{}k_oob_{}_'.format(
                int(pagesiz / 1024), int(blksiz / 1024), oobsiz)
            outimg = os.path.join(prj_out_dir, prefix + imgname)
            if os.path.exists(outimg):
                os.remove(outimg)
            cmdstr = 'python3 ' + aic_script_dir + 'makeuffs.py '
            cmdstr += '--pagesize {} '.format(pagesiz)
            cmdstr += '--oobsize {} '.format(oobsiz)
            cmdstr += '--blockpagecount {} '.format(blkpagecnt)
            cmdstr += '--tooldir {} '.format(aic_script_dir)
            cmdstr += '--inputdir {} '.format(srcdir)
            cmdstr += '--outfile {}\n'.format(outimg)
            mkfscmd += cmdstr
    os.environ["img{}_srcdir".format(img_id)] = srcdir
    return mkfscmd


def get_post_build_bat(aic_root_n, post_objcopy, post_build_cmd):
    des_file = os.path.join(prj_out_dir, './post_build.bat')
    with open(des_file, "w") as f:
        f.write('@echo off\n')
        f.write('setlocal EnableDelayedExpansion\n')
        f.write(post_objcopy)
        f.write(post_build_cmd)
        f.write('echo success\n')
        f.write('pause\n')


def mkimage_prebuild(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig, fsinstall):
    global prj_root_dir
    global aic_script_dir
    global aic_common_dir
    global aic_pack_dir
    global prj_out_dir
    chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
    os.environ["PRJ_OUT_DIR"] = prj_out_dir
    sys.path.append(chip_path)
    ota_enable = 'CONFIG_LPKG_USING_OTA_DOWNLOADER'
    env_enable = 'CONFIG_AIC_ENV_INTERFACE'
    burner_enable = 'CONFIG_GENERATE_BURNER_IMAGE'
    import rtconfig

    POST_ACTION = ''

    if len(fsinstall):
        installcmd = ''
        cleandir = ''
        cleancmd = ''
        for i in range(len(fsinstall)):
            item = fsinstall[i]
            if len(item[0]) == 0 or len(item[1]) == 0 or len(item[2]) == 0:
                continue
            if len(cleandir) > 0:
                cleandir += ',{}'.format(item[2])
            else:
                cleandir = item[2]
            cmdstr = 'python3 ' + aic_script_dir + 'fsinstall.py '
            cmdstr += '--sdkout {} '.format(prj_out_dir)
            cmdstr += '--src {} '.format(item[1])
            cmdstr += '--dst {} \n'.format(item[2])
            installcmd += cmdstr
        cleancmd = 'python3 ' + aic_script_dir + 'fsinstall.py '
        cleancmd += '--sdkout {} '.format(prj_out_dir)
        cleancmd += '--clean {} \n'.format(cleandir)
        POST_ACTION += cleancmd
        POST_ACTION += installcmd

    if prj_kernel != 'baremetal' and "bootloader" != prj_app:
        if get_config(prj_root_dir + '.config', env_enable) == 'y':
            os.system('cp ' + aic_pack_dir + 'env.txt ' + prj_out_dir)

    if not os.path.exists(prj_out_dir):
        os.makedirs(prj_out_dir)
    # Gen partition table
    CMD = 'python3 ' + aic_script_dir + 'gen_partition_table.py'
    CMD += ' -c ' + aic_pack_dir + 'image_cfg.json'
    CMD += ' -o ' + prj_root_dir + 'partition_table.h'
    CMD += ' -j ' + prj_out_dir + 'partition.json'
    os.system(CMD)
    PRE_ACTION = CMD + ';'

    # Gen partition-file-list table
    CMD = 'python3 ' + aic_script_dir + 'gen_partition_file_list.py'
    CMD += ' -c ' + aic_pack_dir + 'image_cfg.json'
    CMD += ' -o ' + prj_out_dir + 'partition_file_list.h'
    os.system(CMD)
    PRE_ACTION += CMD + ';'

    MKFS_ACTION = ''
    MKFS_ACTION += mkimage_gen_mkfs_action(0)
    MKFS_ACTION += mkimage_gen_mkfs_action(1)
    MKFS_ACTION += mkimage_gen_mkfs_action(2)

    # save bootloader
    POST_ACTION += 'python3 ' + aic_script_dir + 'linked_size.py' + ' -m ' \
        + prj_out_dir + rtconfig.SOC + '.map\n'
    SOURCE_BIN = prj_out_dir + rtconfig.SOC + '.bin'
    TARGET_BIN = aic_pack_dir + 'bootloader.bin'
    if prj_kernel == 'baremetal' and "bootloader" == prj_app:
        POST_ACTION += '@cp -r ' + SOURCE_BIN + ' ' + TARGET_BIN + '\n'
        from calc_linked_addr import calc_link_addr
        calc_link_addr(aic_pack_dir + 'image_cfg.json',
                '.config', aic_pack_dir + '.image_cfg.json.tmp')
    else:
        if len(MKFS_ACTION):
            POST_ACTION += MKFS_ACTION
        # packaged ddr files
        POST_ACTION += '@cp -r ' + aic_pack_dir + '* ' + prj_out_dir + '\n'
        if os.path.exists(aic_pack_dir + '.image_cfg.json.tmp'):
            POST_ACTION += '@cp ' + aic_pack_dir + '.image_cfg.json.tmp ' \
                    + prj_out_dir + 'image_cfg.json\n'
        if platform.system() == 'Linux':
            MAKE_DDR_TOOL = 'python3 ' + aic_script_dir + 'mk_private_resource.py'
        elif platform.system() == 'Windows':
            MAKE_DDR_TOOL = aic_script_dir + 'mk_private_resource.exe'
        DDR_JSON = prj_out_dir + 'ddr_init.json'
        PART_JSON = prj_out_dir + 'partition.json'
        DDR_BIN = prj_out_dir + 'ddr_init.bin'
        MAKE_DDR_ACTION = MAKE_DDR_TOOL + \
            ' -v -l {},{} -o {}\n'.format(DDR_JSON, PART_JSON, DDR_BIN)
        if os.path.exists(aic_pack_dir + 'ddr_init.json'):
            POST_ACTION += MAKE_DDR_ACTION
        PBP_JSON = prj_out_dir + 'pbp_cfg.json'
        PBP_BIN = prj_out_dir + 'pbp_cfg.bin'
        MAKE_PBP_ACTION = MAKE_DDR_TOOL + \
            ' -v -l {},{} -o {}\n'.format(PBP_JSON, PART_JSON, PBP_BIN)
        if os.path.exists(aic_pack_dir + 'pbp_cfg.json'):
            POST_ACTION += MAKE_PBP_ACTION
        # pack image files
        if mkimage_check_file_exist(aic_pack_dir, '*.pbp'):
            POST_ACTION += '@cp -r ' + aic_pack_dir + '/*.pbp ' + prj_out_dir + '\n'
        elif mkimage_check_file_exist(aic_common_dir, '*.pbp'):
            POST_ACTION += '@cp -r ' + aic_common_dir + '/*.pbp ' + prj_out_dir + '\n'

        ELF_PARSE_TOOL = 'python3 ' + aic_script_dir + 'elf_parse.py'
        ELF_PARSE_ACTION = ELF_PARSE_TOOL + ' $TARGET ' + prj_out_dir + ' ' + rtconfig.PREFIX + '\n'
        POST_ACTION += ELF_PARSE_ACTION

        MAKE_IMG_TOOL = 'python3 ' + aic_script_dir + 'mk_image.py'
        IMG_JSON = prj_out_dir + 'image_cfg.json'
        if get_config(prj_root_dir + '.config', burner_enable) == 'y':
            MAKE_IMG_ARGS = ' -v -b -c '
        else:
            MAKE_IMG_ARGS = ' -v -c '
        MAKE_IMG_ACTION = MAKE_IMG_TOOL + MAKE_IMG_ARGS + IMG_JSON + ' -d ' + prj_out_dir + '\n'
        POST_ACTION += MAKE_IMG_ACTION

    if get_config(prj_root_dir + '.config', ota_enable) == 'y':
        POST_ACTION += mkimage_cpio(aic_root, prj_chip, prj_kernel, prj_app,
                                    prj_defconfig, rtconfig.OBJCPY)

    # eclipse pre/post build cmd
    aic_root_n = os.path.normpath(aic_root)
    prj_out_dir_n = os.path.normpath(prj_out_dir)
    aic_script_dir_n = os.path.normpath(aic_script_dir)

    eclipse_pre_build = PRE_ACTION
    eclipse_pre_build = eclipse_pre_build.replace(
        'python3', '${ProjDirPath}/../../../tools/env/tools/Python38/python3')
    eclipse_pre_build = eclipse_pre_build.replace(prj_out_dir, '${ProjDirPath}/Debug/')
    eclipse_pre_build = eclipse_pre_build.replace(prj_out_dir_n, '${ProjDirPath}/Debug')
    eclipse_pre_build = eclipse_pre_build.replace(aic_root, '${ProjDirPath}/../../..')
    eclipse_pre_build = eclipse_pre_build.replace(aic_root_n, '${ProjDirPath}/../../..')
    eclipse_pre_build = eclipse_pre_build.replace('\\', '/')

    eclipse_sdk_pre_build = PRE_ACTION
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(
        'python3', '${ProjDirPath}/tools/Python38/python3')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(prj_out_dir, '${ProjDirPath}/Debug/')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(prj_out_dir_n, '${ProjDirPath}/Debug')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(
        aic_script_dir, '${ProjDirPath}/tools/scripts/')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(
        aic_script_dir_n, '${ProjDirPath}/tools/scripts')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(aic_root, '${ProjDirPath}')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(aic_root_n, '${ProjDirPath}')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace('\\', '/')

    eclipse_post_build = POST_ACTION
    eclipse_post_build = eclipse_post_build.replace('\n', ';')
    eclipse_post_build = eclipse_post_build.replace(
        'python3', '${ProjDirPath}/../../../tools/env/tools/Python38/python3')
    eclipse_post_build = eclipse_post_build.replace(
        '@cp', '${ProjDirPath}/../../../tools/env/tools/bin/cp')
    eclipse_post_build = eclipse_post_build.replace(
        '@echo', '${ProjDirPath}/../../../tools/env/tools/bin/echo')
    eclipse_post_build = eclipse_post_build.replace(prj_out_dir, '${ProjDirPath}/Debug/')
    eclipse_post_build = eclipse_post_build.replace(prj_out_dir_n, '${ProjDirPath}/Debug')
    eclipse_post_build = eclipse_post_build.replace(aic_root, '${ProjDirPath}/../../..')
    eclipse_post_build = eclipse_post_build.replace(aic_root_n, '${ProjDirPath}/../../..')
    eclipse_post_build = eclipse_post_build.replace(
        '$TARGET', '${ProjDirPath}/Debug/${ProjName}.elf')
    eclipse_post_build = eclipse_post_build.replace('\\', '/')

    eclipse_sdk_post_build = POST_ACTION
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('\n', ';')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(
        'python3', '${ProjDirPath}/tools/Python38/python3')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('@cp', '${ProjDirPath}/tools/bin/cp')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(
        '@echo', '${ProjDirPath}/tools/bin/echo')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(prj_out_dir, '${ProjDirPath}/Debug/')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(prj_out_dir_n, '${ProjDirPath}/Debug')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(aic_root, '${ProjDirPath}')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(aic_root_n, '${ProjDirPath}')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(
        '$TARGET', '${ProjDirPath}/Debug/${ProjName}.elf')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('\\', '/')

    post_objcopy = "${cross_prefix}${cross_objcopy}${cross_suffix} -O binary \
        ${ProjName}.elf ${ProjName}.bin;"
    os.environ["eclipse_pre_build"] = eclipse_pre_build
    os.environ["eclipse_post_build"] = post_objcopy + eclipse_post_build
    os.environ["eclipse_sdk_pre_build"] = eclipse_sdk_pre_build
    os.environ["eclipse_sdk_post_build"] = post_objcopy + eclipse_sdk_post_build

    get_post_build_bat(prj_out_dir, post_objcopy, eclipse_sdk_post_build)

    # complete flag
    POST_ACTION += '@echo \n@echo Luban-Lite is built successfully \n@echo \n'
    return POST_ACTION


noprj_msg = '''
No project information
You can start one project by following commands

    scons --list-def
    scons --apply-def=<chip>_<board>_<kernel>_<app>_defconfig
    scons
'''


def chk_prj_config(aic_root):
    # cmd-option: list defconfig
    list_def_cmd(aic_root)

    # cmd-option: apply defconfig
    apply_def_cmd(aic_root)

    # cmd-option: list defconfig without bootloader
    list_noboot_def_cmd(aic_root)
    # cmd-option: apply defconfig without bootloader
    apply_noboot_def_cmd(aic_root)

    # cmd-option: add board cmd
    add_board_cmd(aic_root)

    # check '.config'
    path = os.path.join(aic_root, '.config')
    if not os.path.exists(path):
        print(noprj_msg)
        exit(0)

    # update '.config'
    update_defconfig(aic_root)

    # update 'rtconfig.h'
    sys.path.append(os.path.join(aic_root, 'kernel/rt-thread/tools'))
    from menuconfig import mk_rtconfig
    mk_rtconfig(".config")


def get_all_chip(tdir):
    v = []
    for root, dirs, files in os.walk(tdir):
        if root != tdir:
            break
        for d in dirs:
            if d != 'configs':
                v.append(d)
    v.sort()
    return v


def get_all_board(tdir):
    v = []
    for root, dirs, files in os.walk(tdir):
        if root != tdir:
            break
        for d in dirs:
            v.append(d)
    v.sort()
    return v


def get_all_kernel(tdir):
    v = []
    for root, dirs, files in os.walk(tdir):
        if root != tdir:
            break
        for d in dirs:
            if d != 'common':
                v.append(d)
    v.sort()
    return v


def get_all_app(tdir):
    v = []
    for root, dirs, files in os.walk(tdir):
        if root != tdir:
            break
        for d in dirs:
            v.append(d)
    v.sort()
    return v


def mkimage_xip_postaction(aic_root, prj_chip, prj_kernel, prj_app, prj_defconfig, rtconfig_objcpy):
    config_file = os.path.join(aic_root, '.config')
    key_xip = 'CONFIG_AIC_XIP'
    global XIP_POST_ACTION
    XIP_POST_ACTION = ''

    # get CONFIG_AIC_XIP
    AIC_XIP = get_config(config_file, key_xip)
    if AIC_XIP is None:
        return XIP_POST_ACTION

    global prj_out_dir
    prj_name = prj_defconfig.replace('_defconfig', '')
    prj_out_dir = os.path.join(aic_root, 'output/' + prj_name + '/images/')

    # XIP image post action
    XIP_POST_ACTION += '@echo ' + prj_kernel + "-" + prj_app + ': make XIP image begin...\n'
    XIP_POST_ACTION += rtconfig_objcpy + ' -O binary -j .text -j .eh_frame_hdr -j .eh_frame\
        -j .gcc_except_table -j .rodata $TARGET ' + prj_out_dir + prj_chip\
        + '.text.bin\n'
    XIP_POST_ACTION += rtconfig_objcpy + ' -O binary -j .ram.code -j .data $TARGET '\
        + prj_out_dir + prj_chip + '.data.bin\n'

    XIP_POST_ACTION += 'python3 ' + aic_script_dir + 'merge_files.py ' + '-i '\
        + prj_out_dir + prj_chip + '.text.bin' + ' ' + prj_out_dir\
        + prj_chip + '.data.bin' + ' -o ' + prj_out_dir + prj_chip + '.bin\n'
    XIP_POST_ACTION += '@echo ' + prj_kernel + "-" + prj_app + ': make XIP image done... \n'

    return XIP_POST_ACTION


def mkimage_cpio(aic_root, prj_chip, prj_kernel, prj_app, prj_defconfig, rtconfig_objcpy):
    global CPIO_POST_ACTION
    CPIO_POST_ACTION = ''

    if not os.path.exists(aic_pack_dir + 'ota-subimgs.cfg'):
        CPIO_POST_ACTION += '@echo ' + 'Warning: mkimage_cpio not find '\
                + aic_pack_dir + 'ota-subimgs.cfg' + ' \n'
        return CPIO_POST_ACTION

    aic_system = platform.system()

    CPIO_POST_ACTION += '@echo ' + 'make CPIO image begin...\n'
    CPIO_POST_ACTION += 'python3 ' + aic_script_dir + 'mkcpio.py ' + aic_system\
        + ' ' + aic_root + ' ' + aic_pack_dir + ' ' + prj_out_dir + '\n'

    CPIO_POST_ACTION += '@echo ' + 'make CPIO image done...\n'
    return CPIO_POST_ACTION


def get_prj_config(aic_root):
    config_file = os.path.join(aic_root, '.config')
    key_chip = 'CONFIG_PRJ_CHIP'
    key_board = 'CONFIG_PRJ_BOARD'
    key_kernel = 'CONFIG_PRJ_KERNEL'
    key_app = 'CONFIG_PRJ_APP'
    key_lds = 'CONFIG_PRJ_CUSTOM_LDS'

    # get PRJ_CHIP
    PRJ_CHIP = get_config(config_file, key_chip)
    if PRJ_CHIP is None:
        print('Get {} in {} fail, please check your configuration!'.format(key_chip, config_file))
        exit(0)
    PRJ_CHIP = PRJ_CHIP.replace('"', '')
    # check PRJ_CHIP
    path = os.path.join(aic_root, 'target')
    chips = get_all_chip(path)
    if PRJ_CHIP not in chips:
        print('Chip name {} is invalid, please check your configuration!'.format(PRJ_CHIP))
        exit(0)

    # get PRJ_BOARD
    PRJ_BOARD = get_config(config_file, key_board)
    if PRJ_BOARD is None:
        print('Get {} in {} fail, please check your configuration!'.format(key_board, config_file))
        exit(0)
    PRJ_BOARD = PRJ_BOARD.replace('"', '')
    # check PRJ_BOARD
    path = os.path.join(aic_root, 'target', PRJ_CHIP)
    boards = get_all_board(path)
    if PRJ_BOARD not in boards:
        print('Board name {} is invalid, please check your configuration!'.format(PRJ_BOARD))
        exit(0)

    # get PRJ_KERNEL
    PRJ_KERNEL = get_config(config_file, key_kernel)
    if PRJ_KERNEL is None:
        print('Get {} in {} fail, please check your configuration!'.format(key_kernel, config_file))
        exit(0)
    PRJ_KERNEL = PRJ_KERNEL.replace('"', '')
    # check PRJ_KERNEL
    path = os.path.join(aic_root, 'kernel')
    kernels = get_all_kernel(path)
    if PRJ_KERNEL not in kernels:
        print('Kernel name {} is invalid, please check your configuration!'.format(PRJ_KERNEL))
        exit(0)

    # get PRJ_APP
    PRJ_APP = get_config(config_file, key_app)
    if PRJ_APP is None:
        print('Get {} in {} fail, please check your configuration!'.format(key_app, config_file))
        exit(0)
    PRJ_APP = PRJ_APP.replace('"', '')
    # check PRJ_APP
    path = os.path.join(aic_root, 'application', PRJ_KERNEL)
    apps = get_all_app(path)
    if PRJ_APP not in apps:
        print('App name {} is invalid, please check your configuration!'.format(PRJ_APP))
        exit(0)

    # get PRJ_NAME
    PRJ_DEFCONFIG_NAME = get_prj_defconfig(aic_root)

    # get PRJ_CUSTOM_LDS
    custom_lds = get_config(config_file, key_lds)
    if custom_lds:
        custom_lds = custom_lds.replace('"', '')
    else:
        custom_lds = ''
    PRJ_CUSTOM_LDS = custom_lds

    PRJ_TOOLCHAIN_VER = get_config(config_file, 'CONFIG_PRJ_TOOLCHAIN_VER')

    # cmd-option: save defconfig
    save_def_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: project info
    show_info_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: call 'pkgs --update'
    pkgs_update_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: windows menuconfig
    win_menuconfig_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: run qemu target
    run_qemu_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: list mem
    list_mem_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: list module
    list_module_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: list size
    list_size_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: call aicupg command (installed by AiBurn)
    aicupg_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: distclean
    distclean_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # trim toolchain
    trimtoolchain_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # check & prepare toolchain
    chk_prepare_toolchain(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    return (PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME, PRJ_CUSTOM_LDS,
            PRJ_TOOLCHAIN_VER)


def get_prj_post_action(aic_root, prj_chip, prj_board, prj_kernel, prj_app,
                        prj_defconfig, fsinstall):
    global prj_root_dir
    global aic_script_dir
    global aic_common_dir
    global aic_pack_dir
    global prj_out_dir

    prj_root_dir = aic_root + '/'
    aic_script_dir = os.path.join(aic_root, 'tools/scripts/')
    aic_common_dir = os.path.join(aic_root, 'bsp/artinchip/sys/' + prj_chip)
    aic_pack_dir = os.path.join(aic_root, 'target/' + prj_chip + '/' + prj_board + '/pack/')
    prj_name = prj_defconfig.replace('_defconfig', '')
    prj_out_dir = os.path.join(aic_root, 'output/' + prj_name + '/images/')

    # make image pre_build cmd
    POST_ACTION = mkimage_prebuild(aic_root, prj_chip, prj_board, prj_kernel,
                                   prj_app, prj_defconfig, fsinstall)
    return POST_ACTION
