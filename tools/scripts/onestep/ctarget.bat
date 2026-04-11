@echo off
rem SPDX-License-Identifier: Apache-2.0
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

if NOT exist %SDK_PRJ_TOP_DIR%\.config (
	echo Not lunch project yet
	goto ct_exit
)

set /P defconfig=<%SDK_PRJ_TOP_DIR%\.defconfig
for /f "tokens=1* delims=_" %%a in ("%defconfig%") do (
    set chip=%%a
)
for /f "tokens=2* delims=_" %%a in ("%defconfig%") do (
    set board=%%a
)
set target_dir=%SDK_PRJ_TOP_DIR%\target\%chip%\%board%
if exist %target_dir% cd /d %target_dir%

:ct_exit
