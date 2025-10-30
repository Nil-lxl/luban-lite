@echo off
rem SPDX-License-Identifier: Apache-2.0
rem
rem Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd

setlocal enabledelayedexpansion
if NOT exist %SDK_PRJ_TOP_DIR%\.config (
	echo Not lunch project yet
	goto exit_label
)

set /P defconfig=<%SDK_PRJ_TOP_DIR%\.defconfig
set defconfig=!defconfig:~0,-1!

for /f "tokens=3* delims=_" %%a in ("%defconfig%") do (
	set kernel=%%a
)
for /f "tokens=4* delims=_" %%a in ("%defconfig%") do (
	set app=%%a
)

if "%kernel%_%app%" == "baremetal_bootloader" (
    echo Not lunch application yet.
	goto exit_label
)

call scons -C %SDK_PRJ_TOP_DIR% -j 8

:exit_label
endlocal
