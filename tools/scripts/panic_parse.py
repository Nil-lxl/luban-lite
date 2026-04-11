#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

# Copyright (C) 2025, ArtInChip Technology Co., Ltd
# Author: Matteo <duanmt@artinchip.com>

import os
import sys
import argparse
import subprocess
import datetime
import json
import re
import shutil

# Global
SDK_DIR = os.getcwd() + '/'
VERBOSE = False
STAGE_CNT = 0

COLOR_BEGIN = "\033["
COLOR_RED = COLOR_BEGIN + "41;37m"
COLOR_YELLOW = COLOR_BEGIN + "43;30m"
COLOR_WHITE = COLOR_BEGIN + "47;30m"
COLOR_END = "\033[0m"


def cur_date():
    now = datetime.datetime.now()
    tm_str = ('%d-%02d-%02d') % (now.year, now.month, now.day)
    return tm_str


def cur_time():
    now = datetime.datetime.now()
    tm_str = ('%02d:%02d:%02d') % (now.hour, now.minute, now.second)
    return tm_str


def pr_err(string):
    if VERBOSE:
        print(COLOR_RED + '*** ' + cur_time() + ' ' + string + COLOR_END)
    else:
        print(COLOR_RED + '*** ' + string + COLOR_END)


def pr_info(string):
    if VERBOSE:
        print(COLOR_WHITE + '>>> ' + cur_time() + ' ' + string + COLOR_END)
    else:
        print(COLOR_WHITE + '>>> ' + string + COLOR_END)


def pr_warn(string):
    if VERBOSE:
        print(COLOR_YELLOW + '!!! ' + cur_time() + ' ' + string + COLOR_END)
    else:
        print(COLOR_YELLOW + '!!! ' + string + COLOR_END)


def do_pipe(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True)
    return result.stdout.decode('utf-8')


def stage_log(msg):
    print('\n---------------------------------------------------------------')
    print(msg)
    print('---------------------------------------------------------------')


def is_linux():
    return sys.platform.startswith('linux')


def search_special_reg(reg, line):
    try:
        return int(line.strip().replace('0x', '').split(':')[1].strip(), 16)
    except Exception as e:
        return 0


def scan_elf():
    with open('.defconfig', 'r') as f:
        defconfig = f.read().strip()

        soc = defconfig.split('_')[0]
        return os.path.join('output', defconfig, 'images', soc + '.elf')


def check_elf(elf_file):
    with open(elf_file, 'rb') as f:
        elf_header = f.read(4)
        if elf_header != b'\x7fELF':
            pr_err(f'Invalid ELF header: {elf_file}')
            return False
        else:
            print(f'ELF: {elf_file}')
            return True

    pr_warn(f'No ELF file')
    return False


class CPU:
    def __init__(self, name, elf):
        self.name = name
        self.elf_file = elf
        self.regs = {}
        self.tool_addr2line = ''
        self.tool_readelf = ''
        self.exception = 0xFF
        self.text_start = 0
        self.text_end = 0
        self.stack_start = 0
        self.stack_end = 0

    def get_exception(self):
        if self.exception in self.exceptions:
            return self.exceptions[self.exception]
        else:
            return 'Unknown'

    def get_toolchain(self):
        return self.toochain

    def check_toolchain(self):
        if is_linux():
            tool_appendix = ''
        else:
            tool_appendix = '.exe'

        self.tool_addr2line = os.path.join('toolchain', 'bin',
                                           self.get_toolchain() + '-addr2line' + tool_appendix)
        if not os.path.exists(self.tool_addr2line):
            pr_warn(f'Can not found {self.tool_addr2line}...')
            print(f'Please try build Luban-Lite first!')
            return False

        self.tool_readelf = self.tool_addr2line.replace('addr2line', 'readelf')
        print(f'Toolchain: {self.get_toolchain()}')
        return True

    def get_reg_alias(self, reg_name):
        if reg_name in self.reg_alias:
            return self.reg_alias[reg_name]
        else:
            return ''

    def parse_symbol_from_elf(self, symbol):
        if not self.elf_file:
            return 0

        if is_linux():
            tool_grep = 'grep'
        else:
            tool_grep = 'findstr'

        tmp = do_pipe(f'{self.tool_readelf} -s {self.elf_file} | {tool_grep} {symbol}')
        return int(tmp.strip().split()[1], 16)

    def parse_section(self, log):
        with open(log, 'r') as f:
            log = f.read()

        self.text_start = self.parse_symbol_from_elf('__stext')
        self.text_end = self.parse_symbol_from_elf('__etext')

        print('Text section: 0x{:08x} - 0x{:08x}, size: {} Bytes'
              .format(self.text_start, self.text_end, self.text_end - self.text_start))

        match = re.search(r"stack_addr:0x([0-9a-f]+)", log)
        self.stack_start = int(match.group(1), 16) if match else 0

        match = re.search(r"stack_addr_end:0x([0-9a-f]+)", log)
        self.stack_end = int(match.group(1), 16) if match else 0

    def is_code(self, addr):
        if addr >= self.text_start and addr <= self.text_end:
            return True
        else:
            return False

    def add2line(self, addr, elf_file):
        tmp = do_pipe(f'{self.tool_addr2line} -e {elf_file} -fp {hex(addr)}')
        return tmp.strip().replace(SDK_DIR, '')

    def show_cpu_regs(self, log, elf_file):
        if len(self.regs) == 0:
            print('No CPU registers found')
            return False

        stage_log('CPU Registers:')
        print(f'CPU Exception: {self.exception} ({self.get_exception()})')
        if not elf_file:
            return False

        for reg_name, reg_value in self.regs.items():
            if self.is_code(reg_value):
                code = self.add2line(reg_value, elf_file)
            else:
                code = ''

            reg_alias = self.get_reg_alias(reg_name)
            if reg_alias:
                print('{} ({}): 0x{:08x} {}'.format(reg_name, reg_alias, reg_value, code))
            else:
                print('{}: 0x{:08x} {}'.format(reg_name, reg_value, code))
        return True

    def show_call_stack(self, log, elf_file):
        if not elf_file:
            return False

        stage_log('Call Stack:')
        print('Stack section: 0x{:08x} - 0x{:08x}, size: {} Bytes'
              .format(self.stack_start, self.stack_end, self.stack_end - self.stack_start))

        with open(log, 'r') as f:
            log = f.read()

            if 'stack:' not in log:
                pr_warn('No call stack found')
                return False

            stack_log = log.split('stack:')[1]
            addresses = re.findall(r'0x[0-9a-fA-F]+', stack_log)

            cnt = 0
            for address in addresses:
                if self.is_code(int(address, 16)):
                    code = self.add2line(int(address, 16), elf_file)
                    print('{:2}: [{}] {}'.format(cnt, address, code))
                    cnt += 1


class RISCV_CPU(CPU):
    def __init__(self, name, elf):
        super().__init__(name, elf)
        self.toochain = 'riscv64-unknown-elf'
        self.exceptions = {
            0: 'Reserved',
            1: 'Fetch Instruction Access Fault',
            2: 'Illegal Instruction',
            3: 'Breakpoint Fault',
            4: 'Load Instruction Address Misaligned',
            5: 'Load Instruction Access Fault',
            6: 'Store/AMO Address Misaligned',
            7: 'Store/AMO Access Fault',
            8: 'Environment Call from U-Mode',
            9: 'Environment Call from S-Mode',
            11: 'Environment Call from M-Mode',
            12: 'Fetch Instruction page error',
            13: 'Load Instruction page error',
            15: 'Store/AMO Instruction page error',
            24: 'NMI',
        }
        self.reg_alias = {
            'x0': 'zero',
            'x1': 'ra',
            'x2': 'sp',
            'x3': 'gp',
            'x4': 'tp',
            'x5': 't0',
            'x6': 't1',
            'x7': 't2',
            'x8': 's0/fp',
            'x9': 's1',
            'x10': 'a0',
            'x11': 'a1',
            'x12': 'a2',
            'x13': 'a3',
            'x14': 'a4',
            'x15': 'a5',
            'x16': 'a6',
            'x17': 'a7',
            'x18': 's2',
            'x19': 's3',
            'x20': 's4',
            'x21': 's5',
            'x22': 's6',
            'x23': 's7',
            'x24': 's8',
            'x25': 's9',
            'x26': 's10',
            'x27': 's11',
            'x28': 't3',
            'x29': 't4',
            'x30': 't5',
            'x31': 't6',
        }

    def parse_cpu_regs(self, log):
        with open(log, 'r') as f:
            found = False
            for line in f:
                if not found:
                    if "CPU Exception:" in line:
                        match = re.search(r"CPU Exception: NO\.(\d+)", line)
                        self.exception = int(match.group(1)) if match else None
                        found = True
                    continue

                if len(line) < 2:
                    continue

                if 'mcause' in line:
                    self.regs['mcause'] = search_special_reg('mcause', line)
                elif 'mtval' in line:
                    self.regs['mtval'] = search_special_reg('mtval', line)
                elif 'mepc' in line:
                    self.regs['mepc'] = search_special_reg('mepc', line)
                elif 'mstatus' in line:
                    self.regs['mstatus'] = search_special_reg('mstatus', line)
                elif ': ' in line:
                    tmp = re.split('[: \t]', line.strip())
                    tmp = [i for i in tmp if len(i) > 0]
                    for i in range(int(len(tmp)/2)):
                        self.regs[tmp[i * 2]] = int(tmp[i * 2 + 1], 16)

            if not found:
                pr_err(f"Cannot found 'CPU Exception' in {log}")


class CSKY_CPU(CPU):
    def __init__(self, name, elf):
        super().__init__(name, elf)
        self.toochain = 'csky-elf-noneabiv2'
        self.exceptions = {
            0: 'Reset Fault',
            1: 'Instruction Address Misaligned',
            2: 'Instruction Access Fault',
            4: 'Illegal Instruction',
            5: 'Privileged Instruction',
            7: 'Breakpoint Fault',
            8: 'Unrecoverable Fault',
            16: 'TRAP0 Fault',
            17: 'TRAP1 Fault',
            18: 'TRAP2 Fault',
            19: 'TRAP3 Fault',
            22: 'Tspend interrupt',
        }
        self.reg_alias = {
            'r14': 'sp',
            'r15': 'lr',
            'r23': 'fp',
            'r24': 'top',
            'r25': 'bsp',
            'r30': 'svbr',
        }

    def parse_cpu_regs(self, log):
        with open(log, 'r') as f:
            found = False
            for line in f:
                if not found:
                    if "CPU Exception:" in line:
                        match = re.search(r"CPU Exception: NO\.(\d+)", line)
                        self.exception = int(match.group(1)) if match else None
                        found = True
                    continue

                if len(line) < 2:
                    continue

                if 'epsr' in line:
                    self.regs['epsr'] = search_special_reg('epsr', line)
                elif 'epc' in line:
                    self.regs['epc'] = search_special_reg('epc', line)
                elif ': ' in line:
                    tmp = re.split('[: \t]', line.strip())
                    tmp = [i for i in tmp if len(i) > 0]
                    for i in range(int(len(tmp)/2)):
                        self.regs[tmp[i * 2]] = int(tmp[i * 2 + 1], 16)

            if not found:
                pr_err(f"Cannot found 'CPU Exception' in {log}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--cpu", type=str, help="The name of CPU IP")
    parser.add_argument("-e", "--elf", type=str, help="The name of ELF file")
    parser.add_argument("-l", "--log", type=str, help="The name of panic log file")
    parser.add_argument("-v", "--verbose", action='store_true', help="Verbose log")
    args = parser.parse_args()

    if not os.path.exists('tools/onestep.sh'):
        pr_err('Must run the strip in the root director of Luban-Lite!')
        sys.exit(1)

    if args.verbose:
        VERBOSE = True

    if not args.log:
        pr_err('Must given the name of panic log')
        sys.exit(1)

    if not os.path.exists(args.log):
        pr_err(f'{args.log} does not exist')
        sys.exit(1)

    if not args.elf:
        elf_file = scan_elf()
    else:
        elf_file = args.elf
    if not check_elf(elf_file):
        pr_warn("No ELF file found!")
        elf_file = None

    print()
    if not args.cpu:
        print('CPU: RISC-V')
        CPU = RISCV_CPU('riscv', elf_file)
    elif args.cpu.upper() in 'C906/E906/E907':
        print(f'CPU: {args.cpu}')
        CPU = RISCV_CPU(args.cpu.upper(), elf_file)
    elif args.cpu.upper() in 'CK802':
        print(f'CPU: {args.cpu}')
        CPU = CSKY_CPU(args.cpu, elf_file)
    else:
        pr_err(f'Unknown CPU: {args.cpu}')
        sys.exit(1)

    if not CPU.check_toolchain():
        sys.exit(1)

    CPU.parse_cpu_regs(args.log)
    CPU.parse_section(args.log)
    CPU.show_cpu_regs(args.log, elf_file)
    CPU.show_call_stack(args.log, elf_file)
