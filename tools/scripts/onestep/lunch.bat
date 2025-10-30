@echo off
rem SPDX-License-Identifier: Apache-2.0
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

if not "%1" == "" scons --apply-noboot=%1 -C %SDK_PRJ_TOP_DIR%
