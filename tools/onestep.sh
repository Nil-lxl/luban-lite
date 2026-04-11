#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2023-2026 ArtInChip Technology Co., Ltd
# Dehuang Wu <dehuang.wu@artinchip.com>

# Notes:
#
# [In Linux]:
#   1. Change director to top dir of Luban-Lite SDK
#   2. Import onestep.sh:
#       source tools/onestep.sh
#
# [In Windows]:
#   1. Depends on Git Bash, so install Git-2.xx.exe first
#   2. Change current directory to top dir of Luban-Lite SDK
#   3. Import onestep.sh:
#       source tools/onestep.sh
#   4. In Git Bash, there is a problem in menuconfig:
#      The arrow keys do not respond, but the hotkeys does work.

MAX_LINES=20
SDK_PRJ_TOP_DIR=
SDK_CFG_FILE=

backup_list=
display_list=
display_list_total=

# $1 - the command name
function _unalias()
{
	CMD=$1
	alias | grep $CMD"=" -w > /dev/null
	if [ $? -eq 0 ]; then
		unalias $CMD
	fi
}

function _clear_env()
{
	unset ckernel
	_unalias ck
	unset cuboot
	_unalias cu
	unset mm
	unset genindex
	_unalias gi
	unset goexplorer
	_unalias ge
	unset um
}

function _hline()
{
	local cmd="$1"
	local opt="$2"
	local txt="$3"

	if [ "${cmd}" == "" ]; then
		printf "  ${txt}\n"
	elif [ "${opt}" == "" ]; then
		printf "  \e[1;36m%-25s\e[0m : %s\n" "${cmd}" "${txt}"
	else
		printf "  \e[1;36m%-14s\e[0m \e[0;35m%-10s\e[0m : %s\n" "${cmd}" "${opt}" "${txt}"
	fi
}

function hmm()
{
	echo "Luban-Lite SDK OneStep commands:"
	_hline "hmm|h" "" "Get this help."
	_hline "lunch" "[keyword]" "Start with selected defconfig.e.g. lunch mmc"
	_hline "menuconfig|me" "" "Config application with menuconfig"
	_hline "bm" "" "Config bootloader with menuconfig"
	_hline "km" "" "Config application with menuconfig"
	_hline "m|mb" "" "Build bootloader & application and generate final image"
	_hline "ma" "" "Build application only"
	_hline "mu|ms" "" "Build bootloader only"
	_hline "c" "" "Clean bootloader and application"
	_hline "mc" "" "Clean & Rebuild all and generate final image"
	_hline "croot|cr" "" "cd to SDK root directory."
	_hline "cout|co" "" "cd to build output directory."
	_hline "cbuild|cb" "" "cd to build root directory."
	_hline "ctarget|ct" "" "cd to target board directory."
	_hline "csys|cs" "" "cd to chip system directory."
	_hline "godir|gd" "[keyword]" "Go/jump to selected directory."
	_hline "list" "" "List all SDK defconfig."
	_hline "list_module" "" "List all enabled modules."
	_hline "i" "" "Get current project's information."
	_hline "buildall"   "" "Build all the *defconfig in target/configs"
	_hline "rebuildall" "" "Clean and build all the *defconfig in target/configs"
	_hline "addboard|ab" "" "Add new board *defconfig in target/configs"
	_hline "aicupg" "" "Burn image file to target board"
	_hline "update" "" "Manually update some files to be compatible with the new version of SDK."
	echo ""
}
alias h=hmm

# Get the solutions list of ArtInChip
function _get_solution_list()
{
	[[ -z ${SDK_PRJ_TOP_DIR} ]] && {
		return
	}

	echo Built-in configs:
	# Get the solutions list of ArtInChip
	SOLUTION_LIST_ALL=`ls -1 ${SDK_PRJ_TOP_DIR}/target/configs/ | grep "_defconfig$"`
	SOLUTION_LIST_BOOT=`echo "${SOLUTION_LIST_ALL}" | grep bootloader`
	SOLUTION_LIST=`echo "${SOLUTION_LIST_ALL}" | grep -v "bootloader"`
	SOLUTION_LIST_WITH_BOOT=`echo -e "${SOLUTION_LIST_BOOT}\n""${SOLUTION_LIST}"`

	unset SOLUTION_TOTAL
}

cd_root()
{
	if [ ! "$PWD" = "$SDK_PRJ_TOP_DIR" ]; then
		cd $SDK_PRJ_TOP_DIR || exit 110
		GO_BACK=yes
	else
		GO_BACK=no
	fi
}

cd_back()
{
	if [ "$GO_BACK" = "yes" ]; then
		cd - > /dev/null || exit 110
	fi
}

function checkout_binary()
{
	which git 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		return
	fi

	git status 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		return
	fi

	git status | grep "\.a" | while read bin
	do
		bin=$(echo $bin | awk '{print $2}')
		if [ ! -z $bin ] && [ -f $bin ]; then
			echo "Checkout $bin"
			git checkout $bin
		fi
	done
}

function lunch()
{
	local keyword="$*"

	[[ -z ${SDK_PRJ_TOP_DIR} ]] && {
		return
	}

	cd_root
	_get_solution_list

	defconfigs=`echo "${SOLUTION_LIST}"`
	select_item=

	if [ "${keyword}" != "" ]; then
		boot_filter=`echo ${keyword} | grep baremetal_bootloader`
		if [ -n "$boot_filter" ]; then
			select_item=$(echo "${SOLUTION_LIST_WITH_BOOT}" | grep "${keyword}")
		else
			select_item=$(echo "${defconfigs}" | grep "${keyword}")
		fi
	fi
	if [ "${keyword}" == "" ] || [ "${select_item}" != "${keyword}" ]; then
		_display_list_init "${defconfigs}"
		select_item=
		_search_in_list "${keyword}"

		# Not select any defconfig, cancel lunch
		[[ ${select_item} == "" ]] && {
			_display_list_clear
			return
		}
	fi

	_display_list_clear
	echo "Current solution: ${select_item}"
	if [ ${select_item} = "all" ]; then
		echo ${select_item} > $SDK_CFG_FILE
		cd_back
		return
	fi

	echo ${select_item} | awk -F '_def' '{print $1}' > $SDK_CFG_FILE
	scons --apply-def=${select_item}
	cd_back
}

function _get_cur_def()
{
	local cur_def=

	cur_def=$(cat ${SDK_CFG_FILE} | tr -d '\r' |tr -d '\n')
	printf ${cur_def}
}

function croot()
{
	[[ -z ${SDK_PRJ_TOP_DIR} ]] && {
		echo "Not lunch project yet"
		return
	}
	cd ${SDK_PRJ_TOP_DIR} || exit 110
}
alias cr=croot

function cbuild()
{
	local build_dir=
	local cur_def=
	local ret=

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	cur_def=$(_get_cur_def)
	if [ "$cur_def" == "all" ]; then
		return
	fi

	build_dir=${SDK_PRJ_TOP_DIR}/output/$(_get_cur_def)
	if [ -d $build_dir ]; then
		cd ${build_dir} || exit 110
	fi
}
alias cb=cbuild

function get_cur_chip_name()
{
	chip_name=$(cat ${SDK_PRJ_TOP_DIR}/.config | grep CONFIG_PRJ_CHIP | awk -F '"' '{print $2}')
}

function get_cur_board_name()
{
	board_name=$(cat ${SDK_PRJ_TOP_DIR}/.config | grep CONFIG_PRJ_BOARD | awk -F '"' '{print $2}')
}

function csys()
{
	local build_dir=
	local sys_dir=
	local cur_def=
	local ret=

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	cur_def=$(_get_cur_def)
	if [ "$cur_def" == "all" ]; then
		return
	fi

	get_cur_chip_name
	get_cur_board_name
	sys_dir=${SDK_PRJ_TOP_DIR}/bsp/artinchip/sys/${chip_name}
	if [ -d ${sys_dir} ]; then
		cd ${sys_dir} || exit 110
	fi
}
alias cs=csys

function ctarget()
{
	local build_dir=
	local target_dir=
	local cur_def=
	local ret=

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	cur_def=$(_get_cur_def)
	if [ "$cur_def" == "all" ]; then
		return
	fi

	get_cur_chip_name
	get_cur_board_name
	target_dir=${SDK_PRJ_TOP_DIR}/target/${chip_name}/${board_name}
	if [ -d ${target_dir} ]; then
		cd ${target_dir} || exit 110
	fi
}
alias ct=ctarget

function cout()
{
	local cur_def=
	local dst_dir=
	local ret=

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	cur_def=$(_get_cur_def)
	if [ "$cur_def" == "all" ]; then
		return
	fi

	dst_dir=${SDK_PRJ_TOP_DIR}/output/$(_get_cur_def)
	if [ -d $dst_dir ]; then
		cd ${dst_dir} || exit 110
		if [ -d images ]; then
			cd images || exit 110
		fi
	fi
}
alias co=cout

# $1: APP name
# $2 - if clean before make
# Note: Must define the global variable:
#     $BUILD_CNT, $RESULT_FILE, $WARNING_FILE, $LOG_DIR
build_one_solution()
{
	DEFCONFIG_NAME=$1
	NEED_CLEAN=$2

	SOLUTION_NAME=${DEFCONFIG_NAME::-10}
	LOG_FILE=$LOG_DIR/$SOLUTION_NAME".log"
	echo
	echo --------------------------------------------------------------
	if [ -z $SOLUTION_TOTAL ]; then
		echo Build $SOLUTION_NAME
	else
		echo ${SOLUTION_CNT}/${SOLUTION_TOTAL}. Build $SOLUTION_NAME
	fi
	echo --------------------------------------------------------------

	scons --apply-def=$DEFCONFIG_NAME -C $SDK_PRJ_TOP_DIR

	if [ -z $NEED_CLEAN ]; then
		rm -f ${SDK_PRJ_TOP_DIR}/output/$SOLUTION_NAME/images/*.elf
	else
		scons -c -C $SDK_PRJ_TOP_DIR
	fi
	scons -C $SDK_PRJ_TOP_DIR -j 8 2>&1 | tee $LOG_FILE

	BUILD_CNT=`expr $BUILD_CNT + 1`
	SUCCESS=`grep "Luban-Lite is built successfully" $LOG_FILE -wc`
	WAR_CNT=`grep -E "warning:|pinmux conflicts" $LOG_FILE -i | grep "is shorter than expected" -vc`

	if [ $SUCCESS -ne 0 ]; then
		printf "%2s. %-40s is OK. Warning: %s \n" \
			$BUILD_CNT $SOLUTION_NAME $WAR_CNT >> $RESULT_FILE
		if [ $WAR_CNT -gt 0 ]; then
			echo [$SOLUTION_NAME]: >> $WARNING_FILE
			grep -E "warning:|pinmux conflicts" $LOG_FILE -i | grep "is shorter than expected" -v >> $WARNING_FILE
			echo >> $WARNING_FILE
		fi
		return 0
	else
		printf "%2s. %-40s is failed. \n" \
			$BUILD_CNT $SOLUTION_NAME >> $RESULT_FILE
		return 100
	fi
}

# menuconfig for current project
function menuconfig()
{
	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	scons --menuconfig -C $SDK_PRJ_TOP_DIR
}
alias me=menuconfig
alias km=menuconfig

function _boot_menuconfig()
{
	local ret=
	local CUR_DEF=

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	CUR_DEF=$(_get_cur_def)
	boot_filter=`echo ${CUR_DEF} | grep baremetal_bootloader`
	if [ -n "$boot_filter" ]; then
		# Current defconfig is bootloader
		scons --menuconfig -C $SDK_PRJ_TOP_DIR
	else
		BUILD_CNT=0
		LOG_DIR=$SDK_PRJ_TOP_DIR/.log
		RESULT_FILE=$LOG_DIR/result.log
		WARNING_FILE=$LOG_DIR/warning.log
		if [ ! -d $LOG_DIR ]; then
			mkdir $LOG_DIR
		fi
		rm -f $RESULT_FILE $WARNING_FILE

		get_cur_chip_name
		get_cur_board_name
		BL_DEF=${chip_name}_${board_name}_baremetal_bootloader
		if [ -f ${SDK_PRJ_TOP_DIR}/target/configs/${BL_DEF}_defconfig ]; then
			scons --apply-def=${BL_DEF}_defconfig -C $SDK_PRJ_TOP_DIR
			scons --menuconfig -C $SDK_PRJ_TOP_DIR

			# Switch to Application after config
			scons --apply-def=${CUR_DEF}_defconfig -C $SDK_PRJ_TOP_DIR
		else
			echo ${BL_DEF}_defconfig does not exist!
		fi
	fi
}
alias bm=_boot_menuconfig

function _make_boot()
{
	local ret=
	local CUR_DEF=

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	checkout_binary

	CUR_DEF=$(_get_cur_def)
	boot_filter=`echo ${CUR_DEF} | grep baremetal_bootloader`
	if [ -n "$boot_filter" ]; then
		# Current defconfig is bootloader
		echo "Build $SDK_PRJ_TOP_DIR/${CUR_DEF}"
		scons -C $SDK_PRJ_TOP_DIR -j 8
	else
		BUILD_CNT=0
		LOG_DIR=$SDK_PRJ_TOP_DIR/.log
		RESULT_FILE=$LOG_DIR/result.log
		WARNING_FILE=$LOG_DIR/warning.log
		if [ ! -d $LOG_DIR ]; then
			mkdir $LOG_DIR
		fi
		rm -f $RESULT_FILE $WARNING_FILE

		get_cur_chip_name
		get_cur_board_name
		BL_DEF=${chip_name}_${board_name}_baremetal_bootloader
		if [ -f ${SDK_PRJ_TOP_DIR}/target/configs/${BL_DEF}_defconfig ]; then
			build_one_solution ${BL_DEF}_defconfig
			# Switch to Application after build
			scons --apply-def=${CUR_DEF}_defconfig -C $SDK_PRJ_TOP_DIR
		else
			echo ${BL_DEF}_defconfig does not exist!
		fi
	fi
}
alias mu=_make_boot
alias ms=_make_boot

function _make_app()
{
	local ret=
	local CUR_DEF=

	CLEAN_FLAG=$1
	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	CUR_DEF=$(_get_cur_def)
	boot_filter=`echo ${CUR_DEF} | grep baremetal_bootloader`
	if [ -n "$boot_filter" ]; then
		# Current defconfig is bootloader
		echo "Not lunch application yet"
		return
	fi
	echo "Build $SDK_PRJ_TOP_DIR/${CUR_DEF}"
	if [ -n "${CLEAN_FLAG}" ]; then
		scons -c -C $SDK_PRJ_TOP_DIR -j 8
	fi
	scons -C $SDK_PRJ_TOP_DIR -j 8
}
alias ma=_make_app

function _make_boot_and_app()
{
	local ret=
	local CUR_DEF=

	# Clean before build
	CLEAN_FLAG=$1

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	checkout_binary

	CUR_DEF=$(_get_cur_def)
	boot_filter=`echo $CUR_DEF | grep baremetal_bootloader`
	if [ -n "$boot_filter" ]; then
		echo "Build $SDK_PRJ_TOP_DIR/$CUR_DEF"
		scons -C $SDK_PRJ_TOP_DIR -j 8
	else
		BUILD_CNT=0
		LOG_DIR=$SDK_PRJ_TOP_DIR/.log
		RESULT_FILE=$LOG_DIR/result.log
		WARNING_FILE=$LOG_DIR/warning.log
		if [ ! -d $LOG_DIR ]; then
			mkdir $LOG_DIR
		fi
		rm -f $RESULT_FILE $WARNING_FILE

		get_cur_chip_name
		get_cur_board_name
		BL_DEF=${chip_name}_${board_name}_baremetal_bootloader
		if [ -f ${SDK_PRJ_TOP_DIR}/target/configs/${BL_DEF}_defconfig ]; then
			build_one_solution ${BL_DEF}_defconfig ${CLEAN_FLAG}
		fi
		ret=$?
		if [ $ret -eq 0 ]; then
			build_one_solution ${CUR_DEF}_defconfig ${CLEAN_FLAG}
		else
			scons --apply-def=${CUR_DEF}_defconfig -C $SDK_PRJ_TOP_DIR
		fi
	fi
}
alias m=_make_boot_and_app
# Legacy command mb
alias mb=_make_boot_and_app

function _clean_boot_and_app()
{
	local ret=
	local CUR_DEF=

	# Clean before build
	CLEAN_FLAG=$1

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	CUR_DEF=$(_get_cur_def)
	boot_filter=`echo ${CUR_DEF} | grep baremetal_bootloader`
	if [ -n "$boot_filter" ]; then
		echo "Build $SDK_PRJ_TOP_DIR/${CUR_DEF}"
		scons -c -C $SDK_PRJ_TOP_DIR
	else
		BUILD_CNT=0
		LOG_DIR=$SDK_PRJ_TOP_DIR/.log
		RESULT_FILE=$LOG_DIR/result.log
		WARNING_FILE=$LOG_DIR/warning.log
		if [ ! -d $LOG_DIR ]; then
			mkdir $LOG_DIR
		fi
		rm -f $RESULT_FILE $WARNING_FILE

		get_cur_chip_name
		get_cur_board_name
		BL_DEF=${chip_name}_${board_name}_baremetal_bootloader
		if [ -f ${SDK_PRJ_TOP_DIR}/target/configs/${BL_DEF}_defconfig ]; then
			scons --apply-def=${BL_DEF}_defconfig -C $SDK_PRJ_TOP_DIR
			scons -c -C $SDK_PRJ_TOP_DIR
		fi
		scons --apply-def=${CUR_DEF}_defconfig -C $SDK_PRJ_TOP_DIR
		scons -c -C $SDK_PRJ_TOP_DIR
	fi
}
alias c=_clean_boot_and_app

# Clean then Build boot and app
function mc()
{
	m clean
}

# $1 - if clean before make
function build_check_all()
{
	BUILD_CNT=0
	LOG_DIR=$SDK_PRJ_TOP_DIR/.log
	RESULT_FILE=$LOG_DIR/result.log
	WARNING_FILE=$LOG_DIR/warning.log

	if [ ! -d $LOG_DIR ]; then
		mkdir $LOG_DIR
	fi
	rm -f $RESULT_FILE $WARNING_FILE

	_get_solution_list > /dev/null

	SOLUTION_TOTAL=$(echo $SOLUTION_LIST_WITH_BOOT | grep -o defconfig | wc -l)
	SOLUTION_CNT=0
	for app in $SOLUTION_LIST_WITH_BOOT
	do
		SOLUTION_CNT=$(expr $SOLUTION_CNT + 1)
		checkout_binary
		build_one_solution $app $1
	done

	echo
	echo --------------------------------------------------------------
	echo The build result of all solution:
	echo --------------------------------------------------------------
	cat $RESULT_FILE

	if [ -f $WARNING_FILE ]; then
		echo
		echo --------------------------------------------------------------
		echo The warning information of all solution:
		echo --------------------------------------------------------------
		cat $WARNING_FILE
		echo
	fi
}

function buildall()
{
	build_check_all
}

function rebuildall()
{
	build_check_all clean
}

function _all_config_update()
{
	cd_root
	configs=`ls ${SDK_PRJ_TOP_DIR}/target/configs/ -1 | grep defconfig`
	for config in $configs
	do
		scons --apply-def=${config}
		python3 ${SDK_PRJ_TOP_DIR}/tools/scripts/config_update.py
		if [ $? -eq 0 ]; then
			echo "Update ${config} successfully."
			echo
		else
			echo "Update ${config} failed!!!!!!!!!"
			return
		fi
	done
		echo "Update all config successfully."
	cd_back
}
alias updateconfig=_all_config_update

function godir()
{
	local keyword="$*"
	local dir_list=

	[[ -z ${SDK_PRJ_TOP_DIR} ]] && {
		return
	}
	dir_list=$(_get_dir_list)

	_display_list_init "${dir_list}"
	select_item=""
	_search_in_list "${keyword}"
	# change directory
	[[ ! ${select_item} == "" ]] && {
		cd ${SDK_PRJ_TOP_DIR}/${select_item} || exit 100
	}
	_display_list_clear
}
alias gd=godir

function genindex()
{
	local keyword="$*"
	local gen_options=
	local result=
	local gen_path=
	local dir_list1=
	local dir_list2=
	local topdir_tmp=

	gen_options=`printf "application\nbsp\nkernel\ntarget\tools\nall"`

	[[ -z ${SDK_PRJ_TOP_DIR} ]] && {
		return
	}

	if [ "${keyword}" != "" ]; then
		result=$(echo "${gen_options}" | grep "${keyword}")
	fi
	if [ "${result}" == "" ]; then
		_display_list_init "${gen_options}"
		select_item=""
		_search_in_list ""
		# change directory
		if [ ! ${select_item} == "" ]; then
			result="${select_item}"
		else
			_display_list_clear
			return
		fi
	fi

	if [ "${result}" = "all" ]; then
		gen_path=${SDK_PRJ_TOP_DIR}
	else
		gen_path=${SDK_PRJ_TOP_DIR}/${result}
	fi

	printf "Generating directory list ..."
	dir_list1=`find ${gen_path}/ -type d ! -path "*/.*"`
	topdir_tmp=${SDK_PRJ_TOP_DIR//\//\\\/}"\\/"
	dir_list2=`echo "${dir_list1}" | sort | sed 's/'"${topdir_tmp}"'//g'`

	echo "${dir_list2}" >${SDK_PRJ_TOP_DIR}/.dirlist

	printf " Done\n"
	_display_list_clear
}
alias gi=genindex

function _info()
{
	local ret=
	local cur_def=

	ret=$(_lunch_check)
	if [ "${ret}" == "false" ]; then
		echo "Not lunch project yet"
		return
	fi

	cur_def=$(_get_cur_def)
	if [ "$cur_def" == "all" ]; then
		echo Current solution: $cur_def
		return
	fi

	scons --info -C $SDK_PRJ_TOP_DIR
	echo
}
alias i=_info

function list()
{
	[[ -z ${SDK_PRJ_TOP_DIR} ]] && {
		echo "Not lunch project yet"
		return
	}

	_get_solution_list
	for app in $SOLUTION_LIST
	do
		printf "  $app\n"
	done
	echo
}

function addboard()
{
	scons --add-board
}
alias ab=addboard

function aicupg()
{
	scons --aicupg -C $SDK_PRJ_TOP_DIR
}

function list_module()
{
	scons --list-module -C $SDK_PRJ_TOP_DIR
}

function _lunch_check()
{
	[[ -z ${SDK_PRJ_TOP_DIR} ]] && {
		echo "false"
		return
	}

	if [ -f ${SDK_CFG_FILE} ]; then
		echo "true"
	else
		echo "false"
	fi
}

function _get_dir_list()
{
	local dir_list1
	local dir_list2
	local topdir_tmp
	local build_dir
	local sep

	sep="\n"

	dir_list1=`find ${SDK_PRJ_TOP_DIR}/application/ -type d ! -path "*/.*"`
	dir_list1+="${sep}"
	dir_list1+=`find ${SDK_PRJ_TOP_DIR}/bsp/ -type d ! -path "*/.*"`
	dir_list1+="${sep}"
	dir_list1+=`find ${SDK_PRJ_TOP_DIR}/kernel/ -type d ! -path "*/.*"`
	dir_list1+="${sep}"
	dir_list1+=`find ${SDK_PRJ_TOP_DIR}/target/ -type d ! -path "*/.*"`
	dir_list1+="${sep}"

	build_dir=${SDK_PRJ_TOP_DIR}/output/$(_get_cur_def)

	topdir_tmp=${SDK_PRJ_TOP_DIR//\//\\\/}"\\/"
	dir_list2=`echo -e "${dir_list1}" | sort | sed 's/'"${topdir_tmp}"'//g'`

	# If cached directory list exist, load cached list
	if [ -f ${SDK_PRJ_TOP_DIR}/.dirlist ]; then
		local cached_list
		cached_list=`cat ${SDK_PRJ_TOP_DIR}/.dirlist`
		dir_list2=`echo -e "${dir_list2}\n${cached_list}" | sort -u`
	fi
	# echo -e "${dir_list2}" >debug.txt
	echo -e "${dir_list2}"
}

function _mark_topdir()
{
	# User may source this file in SDK top dir, or in onestep.sh dir
	if [ -f tools/onestep.sh ]; then
		SDK_PRJ_TOP_DIR=$(pwd)
	elif [ -f ../tools/onestep.sh ]; then
		SDK_PRJ_TOP_DIR=$(cd .. && pwd)
	else
		echo 'Please "source tools/onestep.sh" in SDK Root directory'
		return
	fi
	SDK_CFG_FILE=$SDK_PRJ_TOP_DIR/.defconfig
}

function _setup_terminal()
{
	# Setup the terminal for the TUI.
	# '\e[?1049h': Use alternative screen buffer.
	# '\e[?7l':    Disable line wrapping.
	printf '\e[?1049h\e[?7l'

	# Hide echoing of user input
	stty -echo
}

function _reset_terminal()
{
	printf "\n"
	# Clear lines
	for ((i=0;i<MAX_LINES;i++)); {
		printf '\e[K'
		printf '\n'
	}
	# Move cursor back to input line
	((JUMPBACK_INPUTLINE=${MAX_LINES}+1))
	printf '\e[%sA' ${JUMPBACK_INPUTLINE}

	# Reset the terminal to a useable state (undo all changes).
	# '\e[K':     Clear line
	# '\e[?7h':   Re-enable line wrapping.
	# '\e[?25h':  Unhide the cursor.
	# '\e[2J':    Clear the terminal.
	# '\e[;r':    Set the scroll region to its default value.
	#             Also sets cursor to (0,0).
	# '\e[?1049l: Restore main screen buffer.
	printf '\e[K\e[?7h\e[?25h'
	# Show user input.
	stty echo
}

function _get_term_size()
{
	local max_items=
	local IFSBK=

	MAX_LINES=20
	IFSBK=${IFS}
	IFS=$' \t\n'
	# Get terminal size ('stty' is POSIX and always available).
	# This can't be done reliably across all bash versions in pure bash.
	read -r LINES COLUMNS < <(stty size)
	IFS=${IFSBK}

	# Max list items that fit in the scroll area.
	((max_items=LINES-3))
	((MAX_LINES>max_items)) &&
		MAX_LINES=${max_items}
}

function _arrow_key()
{
	case ${1} in
		# Scroll down.
		# 'B' is what bash sees when the down arrow is pressed
		# ('\e[B' or '\eOB').
		$'\e[B'|\
		$'\eOB')
			((scroll<display_list_total)) && {
				((scroll++))
				_redraw
			}
		;;

		# Scroll up.
		# 'A' is what bash sees when the up arrow is pressed
		# ('\e[A' or '\eOA').
		$'\e[A'|\
		$'\eOA')
			((scroll>0)) && {
				((scroll--))
				_redraw
			}
		;;
	esac
}

function _key_loop()
{
	local input_kw="${2}"
	local new_key
	local array

	select_item=""
	while IFS= read -rsn 1 -p $'\r\e[K'"${1}${input_kw}" new_key; do

		[[ ${new_key} == $'\e' ]] && {
			read "${read_flags[@]}" -rsn 2 new_key2

			# Esc key to exit
			[[ ${new_key2} == "" ]] && return
			_arrow_key ${new_key}${new_key2}
			continue
		}

		case ${new_key} in
			# Backspace.
			$'\177'|$'\b')
				input_kw=${input_kw%?}
			;;

			# Enter/Return/Tab
			""|$'\t')
				array=(${display_list[@]})
				select_item=${array[$scroll]}
				return
			;;
			# Anything else, add it to read reply.
			*)
				input_kw+=${new_key}
			;;
		esac

		# Filter with keyword
		_update_display_with_kw "${input_kw}"

		scroll=0
		_redraw
	done

}

function _draw_line()
{
	# Format the list item and print it.
	local file_name=$2
	local format

	# If the list item is under the cursor.
	(($1 == scroll)) && format+="\\e[1;36;7m"

	# Escape the directory string.
	# Remove all non-printable characters.
	file_name=${file_name//[^[:print:]]/^[}

	# Clear line before changing it.
	printf '\e[K'
	printf '%b%s\e[m\n' "  ${format}" "${file_name}"
}

function _update_display_with_kw()
{
	local input_kw="$1"
	local kw=
	local match_list=

	# Filter with keyword
	if [ ! -z "${input_kw}" ]; then
		kw=${input_kw// /.*}
		kw=${kw//\\/.*}
		kw=${kw//\//\\\/}
		match_list=`echo "${backup_list}" | sed -n '/'"${kw}"'/p'`
		# debug
		# echo "${match_list}" >match.list
		display_list=(${match_list[@]})
		# ((display_list_total=${#display_list[@]}-1))
		((display_list_total=${#display_list[@]}))

	else
		match_list=${backup_list}
		display_list=(${match_list[@]})
		# ((display_list_total=${#display_list[@]}-1))
		((display_list_total=${#display_list[@]}))
	fi

}

function _display_list_init()
{
	local input_list="$1"

	# Save the original data in a second list as a backup.
	backup_list="${input_list}"
	_update_display_with_kw ""
}

function _display_list_clear()
{
	backup_list=""
	unset display_list
	display_list_total=0
}

function _redraw()
{
	# If no content in list, don't draw it
	[[ -z ${backup_list} ]] && return

	start_item=0
	((scroll>=MAX_LINES)) && {
		((start_item=${scroll}-${MAX_LINES}+1))
	}

	((end_item=${start_item}+${MAX_LINES}))

	# '\e[?25l': Hide the cursor.
	printf '\e[?25l\n'
	for ((i=start_item;i<end_item;i++)); {
		if [ ${i} -le ${display_list_total} ]; then
			_draw_line $i ${display_list[i]}
		else
			_draw_line $i ""
		fi
	}

	# '\e[NA':  Move cursor up N line
	# '\e[?25h': Unhide the cursor.
	printf '\e[21A\e[?25h>'
}

function _search_in_list()
{
	local keyword="$*"
	local match_list

	# Trap the exit signal (we need to reset the terminal to a useable state.)
	trap '_reset_terminal' EXIT

	# Trap the window resize signal (handle window resize events).
	trap '_get_term_size; _redraw' WINCH

	# bash 5 and some versions of bash 4 don't allow SIGWINCH to interrupt
	# a 'read' command and instead wait for it to complete. In this case it
	# causes the window to not redraw on resize until the user has pressed
	# a key (causing the read to finish). This sets a read timeout on the
	# affected versions of bash.
	# NOTE: This shouldn't affect idle performance as the loop doesn't do
	# anything until a key is pressed.
	# SEE: https://github.com/dylanaraps/fff/issues/48
	((BASH_VERSINFO[0] > 3)) &&
		read_flags=(-t 0.05)
	_get_term_size

	[[ ! -z "${keyword}" ]] && {
		_update_display_with_kw "${keyword}"
	}
	scroll=0
	_redraw

	_key_loop "> " "${keyword}"
	_reset_terminal
}

function _sdk_update()
{
    python3 ${SDK_PRJ_TOP_DIR}/tools/scripts/sdk_update.py
}
alias update=_sdk_update

_clear_env
_mark_topdir

uname -a | grep MINGW  > /dev/null
if [ $? -eq 0 ]; then
	echo In Windows, need define some environment variable ...
	export PYTHONIOENCODING=UTF-8
	export ENV_ROOT=$SDK_PRJ_TOP_DIR/tools/env
	export PKGS_ROOT=$ENV_ROOT/packages
	export RTT_ROOT=$SDK_PRJ_TOP_DIR/kernel/rt-thread
	export PATH=$ENV_ROOT/tools/Python27:$PATH
	export PATH=$ENV_ROOT/tools/Python27/Scripts:$PATH
	export PATH=$ENV_ROOT/tools/Python38:$PATH
	export PATH=$ENV_ROOT/tools/bin:$PATH
fi
