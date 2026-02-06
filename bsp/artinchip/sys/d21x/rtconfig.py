# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2022-2026, ArtInChip Technology Co., Ltd

import os
import platform
from aic_build import get_config

# toolchains options
# CPUNAME = c906/c906d/c906fd/c906fdv/c906v
SOC         ='d21x'
ARCH        ='risc-v'
CPU         ='c906'
CPUNAME     ='c906fd'
VENDOR      ='artinchip'
CROSS_TOOL  ='gcc'

if os.getenv('RTT_CC'):
    CROSS_TOOL = os.getenv('RTT_CC')

if  CROSS_TOOL == 'gcc':
    PLATFORM    = 'gcc'
    fcwd = os.path.abspath(os.path.dirname(__file__))
    if platform.system() == 'Linux':
        EXEC_PATH = os.path.normpath(fcwd + '/../../../../toolchain/bin/')
    elif platform.system() == 'Windows':
        EXEC_PATH = os.path.normpath(fcwd + '/../../../../toolchain/bin/')
else:
    print ('Please make sure your toolchains is GNU GCC!')
    exit(0)

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

B_AFLAGS = ''
B_CFLAGS = ''
LFLAGS = ''
# BUILD = 'debug'
BUILD = 'release'
# BACKTRACE = True
BACKTRACE = False

if BUILD == 'debug':
    CFLAGS_DBG = ' -O0 -gdwarf-2'
    AFLAGS_DBG = ' -gdwarf-2'
    if BACKTRACE:
        B_AFLAGS += ' -D_ENABLE_BACK_TRACE_STACK_ '
        CFLAGS_DBG += ' -fno-omit-frame-pointer '
        B_AFLAGS += ' -D_NO_OMIT_FRAME_POINT_ '
else:
    CFLAGS_DBG = ' -O2 -g2'
    AFLAGS_DBG = ''
    if BACKTRACE:
        B_AFLAGS += ' -D_ENABLE_BACK_TRACE_STACK_ '

prj_out_dir = ''
if os.environ.get('PRJ_OUT_DIR'):
    prj_out_dir = os.environ.get('PRJ_OUT_DIR')

if PLATFORM == 'gcc':
    # toolchains
    PREFIX  = 'riscv64-unknown-elf-'
    CC      = PREFIX + 'gcc'
    CXX     = PREFIX + 'g++'
    AS      = PREFIX + 'gcc'
    AR      = PREFIX + 'ar'
    LINK    = PREFIX + 'g++'
    TARGET_EXT = 'elf'
    SIZE    = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY  = PREFIX + 'objcopy'
    STRIP   = PREFIX + 'strip'
    LD_SCRIPT       = 'gcc_aic.ld'
    QEMU_LD_SCRIPT  = 'gcc_qemu.ld'

    PRJ_TOOLCHAIN_VER = os.getenv('PRJ_TOOLCHAIN_VER')
    if not PRJ_TOOLCHAIN_VER:
        PRJ_TOOLCHAIN_VER = 'V2.6.1'
    else:
        PRJ_TOOLCHAIN_VER = PRJ_TOOLCHAIN_VER.replace('"', '')

    if PRJ_TOOLCHAIN_VER == 'V2.6.1':
        ISA_TAG = ''
    else:
        ISA_TAG = '_zifencei'

    if CPUNAME == 'c906':
        DEVICE = ' -march=rv64imac' + ISA_TAG + '_xtheadc -mabi=lp64 -mcmodel=medlow'
    if CPUNAME == 'c906d':
        DEVICE = ' -march=rv64imafdc' + ISA_TAG + '_xtheadc -mabi=lp64d -mcmodel=medlow'
    if CPUNAME == 'c906fd':
        DEVICE = ' -march=rv64imafdc' + ISA_TAG + '_xtheadc -mabi=lp64d -mcmodel=medany'
        TOOLCHAIN_LIB = 'rv64imafdc_zfh' + ISA_TAG + '_xtheadc'
        M_DEVICE = ' -march=rv64imafdc -mabi=lp64d -mcmodel=medany'
        M_TOOLCHAIN_LIB = 'rv64imafdc'
    if CPUNAME == 'c906fdv':
        DEVICE = ' -march=rv64imafdcv0p7_zfh' + ISA_TAG + '_xtheadc -mabi=lp64d -mcmodel=medany'
    if CPUNAME == 'c906v':
        DEVICE = ' -march=rv64imafdcv' + ISA_TAG + '_xtheadc -mabi=lp64dv -mcmodel=medany'

    B_CFLAGS += ' -c -g -ffunction-sections -fdata-sections -Wall'
    B_AFLAGS += ' -c' + ' -x assembler-with-cpp' + ' -D__ASSEMBLY__'
    CFLAGS = DEVICE + B_CFLAGS + CFLAGS_DBG + ' -mno-dup-loop-header'
    AFLAGS = DEVICE + B_AFLAGS + AFLAGS_DBG
    CXXFLAGS = CFLAGS
    CFLAGS_GCC14 = ' -Wno-error=implicit-function-declaration -Wno-error=int-conversion \
                    -Wno-error=incompatible-pointer-types -Wno-error=return-mismatch \
                    -Wno-error=declaration-missing-parameter-type -Wno-error=implicit-int'
    if PRJ_TOOLCHAIN_VER == 'V2.6.1':
        CFLAGS_GCC14 = ''
    CFLAGS += CFLAGS_GCC14
    LFLAGS += DEVICE

    PRJ_KERNEL = os.getenv('PRJ_KERNEL')
    if PRJ_KERNEL == 'rt-thread':
        LFLAGS += ' -nostartfiles -Wl,--no-whole-archive -lgcc -Wl,-gc-sections '
    else:
        LFLAGS += ' -nostartfiles -Wl,--no-whole-archive -lm -lc -lgcc -Wl,-gc-sections '
    LFLAGS += ' -Wl,-zmax-page-size=1024'
    LFLAGS += ' -Wl,-Map=' + prj_out_dir + SOC + '.map'
    CPATH = ''
    LPATH = ''

    # module setting
    M_PREFIX = 'riscv-none-embed-'
    M_CC = M_PREFIX + 'gcc'
    M_CXX = M_PREFIX + 'g++'
    M_AS = M_PREFIX + 'gcc'
    M_AR = M_PREFIX + 'ar'
    M_LINK = M_PREFIX + 'g++'
    M_SIZE = M_PREFIX + 'size'
    M_OBJDUMP = M_PREFIX + 'objdump'
    M_OBJCPY = M_PREFIX + 'objcopy'
    M_STRIP = M_PREFIX + 'strip'
    M_CFLAGS = M_DEVICE + B_CFLAGS + CFLAGS_DBG + ' -fPIC -shared'
    M_AFLAGS = M_DEVICE + B_AFLAGS + AFLAGS_DBG
    M_CXXFLAGS = M_CFLAGS
    M_CFLAGS += CFLAGS_GCC14
    M_LFLAGS = M_DEVICE + ' -Wl,--gc-sections,-z,max-page-size=0x4'
    M_LFLAGS += ' -shared -fPIC -nostartfiles -nostdlib -static-libgcc'
    M_POST_ACTION = M_STRIP + ' -R .hash $TARGET\n' + M_SIZE + ' $TARGET \n'
    M_BIN_PATH = ''

DUMP_ACTION = OBJDUMP + ' -D -S $TARGET > ' + prj_out_dir + SOC + '.asm\n'
POST_ACTION = '@ls -og $TARGET\n@echo\n'
POST_ACTION += OBJCPY + ' -O binary $TARGET ' + prj_out_dir + SOC + '.bin\n'
#POST_ACTION += DUMP_ACTION
POST_ACTION += SIZE + ' $TARGET \n'

