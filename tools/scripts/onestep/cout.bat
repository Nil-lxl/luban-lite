@echo off
rem SPDX-License-Identifier: Apache-2.0
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

if NOT exist %SDK_PRJ_TOP_DIR%\.config (
	echo Not lunch project yet
	goto co_exit
)

set /P defconfig=<%SDK_PRJ_TOP_DIR%\.defconfig
set build_dir=%SDK_PRJ_TOP_DIR%\output\%defconfig%
if exist %build_dir% cd /d %build_dir% && cd images

:co_exit
