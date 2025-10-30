@echo off
rem SPDX-License-Identifier: Apache-2.0
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

echo.
echo Luban-Lite SDK OneStep commands:
echo   h             : Get this help.
echo   lunch [No.]   : Start with selected defconfig, .e.g. lunch 3
echo   me            : Config SDK with menuconfig
echo   bm            : Config bootloader with menuconfig
echo   km            : Config application with menuconfig
echo   m/mb          : Build bootloader ^& app and generate final image
echo   ma            : Build application only
echo   mu/ms         : Build bootloader only
echo   mc            : Clean ^& Build all and generate final image
echo   c             : Clean all
echo   croot/cr      : cd to SDK root directory.
echo   cout/co       : cd to build output directory.
echo   cbuild/cb     : cd to build root directory.
echo   ctarget/ct    : cd to target board directory.
echo   list          : List all SDK defconfig.
echo   list_module   : List all enabled modules.
echo   i             : Get current project's information.
echo   buildall      : Build all the *defconfig in target/configs
echo   rebuildall    : Clean and build all the *defconfig in target/configs
echo   aicupg        : Burn image file to target board
echo   update        : Manually update some files to be compatible with the new version of SDK.
echo.
