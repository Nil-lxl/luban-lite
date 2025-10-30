@echo off
rem SPDX-License-Identifier: Apache-2.0
rem
rem Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd

set clean_flag=%1
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

:: (1) Build bootloader
call scons --apply-def=%boot_defconfig% -C %SDK_PRJ_TOP_DIR%

if "%clean_flag%" == "clean" (
	call scons -c -C %SDK_PRJ_TOP_DIR%
)
call scons -C %SDK_PRJ_TOP_DIR%	 -j 8 2>&1 | tee %LOG_DIR%\%boot_defconfig%.log

if "%kernel%_%app%" == "baremetal_bootloader" (
	goto exit_label
)

:: Two cases:
:: 1. Git bash is not included in ENV, use DOS find
:: 2. Git bash is included in ENV, use grep, otherwise it will use find in the bash, but parameter
::    is wrong
where grep > nul 2>&1
if not %errorlevel% equ 0 (
	find "Luban-Lite is built successfully" %LOG_DIR%\%boot_defconfig%.log > nul
) else (
	grep "Luban-Lite is built successfully" %LOG_DIR%\%boot_defconfig%.log > nul
)
if not %errorlevel% equ 0 (
	call scons --apply-def=%app_defconfig% -C %SDK_PRJ_TOP_DIR%
	goto exit_label
)

:: (2) Build app
call scons --apply-def=%app_defconfig% -C %SDK_PRJ_TOP_DIR%

if "%clean_flag%" == "clean" (
	call scons -c -C %SDK_PRJ_TOP_DIR%
)
call scons -C %SDK_PRJ_TOP_DIR% -j 8

:exit_label
endlocal
