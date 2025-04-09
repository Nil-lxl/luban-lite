@echo off
rem SPDX-License-Identifier: Apache-2.0
rem
rem Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd

if "%CD%"=="C:\Windows\system32" (
    echo Current user is administrator, need change path to Luban-Lite ...
    cd /d %~dp0
)

set PYTHONIOENCODING=UTF-8
set SDK_PRJ_TOP_DIR=%cd%
set PATH=%SDK_PRJ_TOP_DIR%\tools\env\tools\bin;%PATH%
set PATH=%SDK_PRJ_TOP_DIR%\tools\env\tools\Python27\Scripts;%PATH%
set PATH=%SDK_PRJ_TOP_DIR%\tools\env\tools\Python38;%PATH%
set PATH=%SDK_PRJ_TOP_DIR%\tools\scripts\onestep;%PATH%

set ENV_ROOT=%SDK_PRJ_TOP_DIR%\tools\env
set PKGS_ROOT=%ENV_ROOT%\packages
set RTT_ROOT=%SDK_PRJ_TOP_DIR%\kernel\rt-thread

rem OneStep command
doskey list=scons --list-noboot -C %SDK_PRJ_TOP_DIR%
doskey bm=boot_menuconfig
doskey km=app_menuconfig
doskey m=make_boot_and_app
doskey mb=make_boot_and_app
:: make uboot
doskey mu=make_boot
:: make spl
doskey ms=make_boot
doskey ma=make_app
doskey c=clean_boot_and_app
doskey mc=make_boot_and_app clean
doskey i=scons --info -C %SDK_PRJ_TOP_DIR%
doskey aicupg=scons --aicupg -C %SDK_PRJ_TOP_DIR%
doskey croot=cd /d %SDK_PRJ_TOP_DIR%
doskey cr=cd /d %SDK_PRJ_TOP_DIR%
doskey cb=cbuild
doskey co=cout
doskey ct=ctarget
doskey update=sdk_update

chcp 65001 > nul

if NOT "%3" == "" (
	cmd.exe /K "%2 %3"
	exit /b %errorlevel%
)
if NOT "%2" == "" (
	cmd.exe /K "%2"
	exit /b %errorlevel%
)

echo Luban-Lite command for Windows
echo.
echo      _         _   ___        ___ _     _
echo     / \   _ __^| ^|_^|_ _^|_ __  / __^| ^|__ (_)_ __
echo    / _ \ ^| '__^| __^|^| ^|^| '_ \^| ^|  ^| '_ \^| ^| '_ \
echo   / ___ \^| ^|  ^| ^|_ ^| ^|^| ^| ^| ^| ^|__^| ^| ^| ^| ^| ^|_) ^|
echo  /_/   \_\_^|   \__^|___^|_^| ^|_^|\___^|_^| ^|_^|_^| .__/
echo                                          ^|_^|
echo.

cmd.exe
