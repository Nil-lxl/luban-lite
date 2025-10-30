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

set LOG_DIR=%SDK_PRJ_TOP_DIR%\.log
if not exist %LOG_DIR% mkdir %LOG_DIR%
del %LOG_DIR%\*.log /f /q

:: Switch to bootloader
call scons --apply-def=%boot_defconfig% -C %SDK_PRJ_TOP_DIR%
call scons --menuconfig -C %SDK_PRJ_TOP_DIR%

:: If it is bootloader project, no need to restore
if "%kernel%_%app%" == "baremetal_bootloader" (
	goto exit_label
)

:: Restore app defconfig
call scons --apply-def=%app_defconfig% -C %SDK_PRJ_TOP_DIR%

:exit_label
endlocal
