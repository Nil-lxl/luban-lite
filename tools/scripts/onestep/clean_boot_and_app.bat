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

for /f "tokens=1* delims=_" %%a in ("%defconfig%") do (
	set chip=%%a
)
for /f "tokens=2* delims=_" %%a in ("%defconfig%") do (
	set board=%%a
)
for /f "tokens=3* delims=_" %%a in ("%defconfig%") do (
	set kernel=%%a
)
for /f "tokens=4* delims=_" %%a in ("%defconfig%") do (
	set app=%%a
)

set app_defconfig=!defconfig!_defconfig
set boot_defconfig=!chip!_!board!_baremetal_bootloader_defconfig

:: (1) Clean bootloader
call scons --apply-def=%boot_defconfig% -C %SDK_PRJ_TOP_DIR%
call scons -c -C %SDK_PRJ_TOP_DIR%

if "%kernel%_%app%" == "baremetal_bootloader" (
	goto exit_label
)

:: (2) Clean app
call scons --apply-def=%app_defconfig% -C %SDK_PRJ_TOP_DIR%
call scons -c -C %SDK_PRJ_TOP_DIR% -j 8

:exit_label
endlocal
