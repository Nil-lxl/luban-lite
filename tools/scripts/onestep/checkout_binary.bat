@echo off
rem SPDX-License-Identifier: Apache-2.0
rem
rem Copyright (C) 2026 ArtInChip Technology Co., Ltd

git --version >nul 2>&1
if errorlevel 1 (
	goto :eof
)

git status >nul 2>&1
if errorlevel 1 (
	goto :eof
)

for /f "tokens=2" %%a in ('git status ^| findstr "\.a"') do (
	if exist "%%a" (
		echo Checkout %%a
		git checkout %%a
	)
)
