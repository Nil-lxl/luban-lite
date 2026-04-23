#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 ArtInChip Technology Co., Ltd
#
# Matteo <duanmt@artinchip.com>

# This script only supports Ubuntu system.

# $1 - 'quiet', run the install with no question

MY_NAME=$0
TOPDIR=$PWD
SDK_NAME=Luban-Lite

TRUE=1
FALSE=0

# Configuration
INTERNET_IS_AVAILABLE=$FALSE
QUIET_MODE=$FALSE

# Return values
ERR_CANCEL=100
ERR_UNSUPPORTED=110
ERR_PKG_UNVAILABLE=111
ERR_NET_UNVAILABLE=112

COLOR_BEGIN="\033["
COLOR_RED="${COLOR_BEGIN}41;37m"
COLOR_YELLOW="${COLOR_BEGIN}43;30m"
COLOR_WHITE="${COLOR_BEGIN}47;30m"
COLOR_END="\033[0m"

pr_err()
{
	echo -e "${COLOR_RED}*** $*${COLOR_END}"
}

pr_warn()
{
	echo -e "${COLOR_YELLOW}!!! $*${COLOR_END}"
}

pr_info()
{
	echo -e "${COLOR_WHITE}>>> $*${COLOR_END}"
}

run_cmd()
{
	echo
	pr_info $1
	echo
	eval $1 || exit 120
}

input_an_answer()
{
	if [ $QUIET_MODE -eq $TRUE ]; then
		echo Y
		ANSWER="Y"
	else
		read ANSWER
	fi
}

check_root()
{
	CUR_USER=$(whoami)
	if [ $CUR_USER = "root" ]; then
		pr_info "Current user is already root"
		return
	fi
	sudo -l -U "$(whoami)" | grep -q ALL
	if [ $? -eq 0 ]; then
		pr_info "Sudo is available"
		return
	fi

	pr_warn $MY_NAME "must install package with 'sudo'. "
	pr_warn "Your password will be safe and always used locally."
}

check_os()
{
	if [ -f /etc/lsb-release ]; then
		OS_VER=`cat /etc/lsb-release | grep RELEASE | awk -F '=' '{print $2}'`
		OS_TYPE="Ubuntu"
	else
		pr_err "Unsupported system OS. This script only supports Ubuntu."
		exit $ERR_UNSUPPORTED
	fi
	pr_info "Current system is $OS_TYPE-$OS_VER"
}

# $1 - the tool name
pkg_is_too_old()
{
	pr_warn "Please install a newer $1 manually, then try $MY_NAME again"
}

pkg_is_ok()
{
	printf "\t\t\t\t\t\t\t\t[OK]\n"
}

# $1 - the package name
pkg_is_failed()
{
	pr_warn "Failed to install $1, please check the install log."
	INSTALL_RESULT=$FALSE
}

# $1 - the command string
check_pkg_src()
{
	pr_info "Try to access the package source ..."
	$1
	if [ $? -ne 0 ]; then
		pr_err "The software source is not accessable! Please check it"
		pr_err "$MY_NAME must download package from a software source."
		exit $ERR_NET_UNVAILABLE
	fi
}

apt_install_tzone()
{
	OS_MAIN_VER=${OS_VER:0:2}
	if [ $OS_MAIN_VER -lt 24 ]; then
		return
	fi

	if [ -f /etc/localtime ] && [ -f /etc/timezone ]; then
		pr_info "Timezone is already set to $(cat /etc/timezone)"
		run_cmd "dpkg --configure -a"
		return
	fi

	echo "Asia/Shanghai" > /etc/timezone
	DEBIAN_FRONTEND=noninteractive apt install -y tzdata

	pr_info "Set timezone to Asia/Shanghai ..."
	run_cmd "dpkg --configure -a"
}

# $1 - package name
# $2 - need user confirmed
apt_install_pkg()
{
	PKG=$1
	CONFIRM=$2

	echo
	pr_info "Check $PKG ..."
	dpkg -s $PKG 2>&1 | grep -E "Status|Version"
	if [ $? -eq 0 ]; then
		pkg_is_ok
		return 0
	fi

	if [ "x$CONFIRM" = "x" ]; then
		ANSWER="Y"
	else
		pr_warn "Will download and install $PKG, continue? Y/N"
		input_an_answer
	fi

	if [ $ANSWER = "Y" ] || [ $ANSWER = "y" ]; then
		pr_info "Try to install $PKG ..."
		apt-get install -y $PKG
		RET=$?
		if [ $RET -ne 0 ]; then
			pkg_is_failed $PKG
		fi
		return $RET
	else
		exit $ERR_CANCEL
	fi
}

apt_install_python3()
{
	if [ -f /usr/bin/python3 ]; then
		pr_info "Python3 is already installed"
		CUR_VER=`python3 -V | awk '{print $2}'`
		SUB_VER=`echo $CUR_VER | awk -F '.' '{print $2}'`
		if [ "$SUB_VER" -lt 6 ]; then
			pr_warn "Python3 version must >= 3.6"
			pkg_is_too_old "Python3-"$CUR_VER
			pkg_is_failed "Python3"
		else
			echo "Python3 version is $CUR_VER"
			pkg_is_ok
		fi
	else
		apt_install_pkg "python3" ask
	fi
}

pip_install_pkg()
{
	if ! command -v python &> /dev/null; then
		update-alternatives --install /usr/bin/python python /usr/bin/python3 10
	fi

	apt_install_pkg "python3-pip"

	OS_MAIN_VER=${OS_VER:0:2}
	if [ $OS_MAIN_VER -gt 22 ]; then
		export PIP_BREAK_SYSTEM_PACKAGES=1
	fi

	cd $TOPDIR/tools/env/local_pkgs || exit $ERR_CANCEL

	PKGS=("scons" "pycryptodomex")
	for i in "${PKGS[@]}"
	do
		PKG_NAME=$(ls ${i}*.tar.gz)
		pr_info "Try to install $PKG_NAME ..."
		pip3 install $PKG_NAME
		if [ $? -ne 0 ]; then
			pkg_is_failed
		fi
	done

	cd - > /dev/null || exit $ERR_CANCEL
}

ubuntu_install()
{
	check_pkg_src "apt-get update"

	apt_install_tzone

	NEED_CONFIRM=("build-essential" "gcc")
	for i in "${NEED_CONFIRM[@]}"
	do
		apt_install_pkg $i ask
	done

	apt_install_python3

	PKGS=("cpio" "libncurses-dev")
	for i in "${PKGS[@]}"
	do
		apt_install_pkg $i
	done

	pip_install_pkg
}

# Main execution
check_root
check_os

if [ "x$1" = "xquiet" ]; then
	QUIET_MODE=$TRUE
fi

if [ ! -f tools/onestep.sh ] || [ ! -d bsp/artinchip/sys/ ]; then
	pr_err "Please run this script in the root directory of $SDK_NAME"
	exit $ERR_UNSUPPORTED
fi

INSTALL_RESULT=$TRUE
if [ $OS_TYPE = "Ubuntu" ]; then
	ubuntu_install
else
	pr_err "Unsupported OS. This script only supports Ubuntu."
	exit $ERR_UNSUPPORTED
fi

echo
if [ $INSTALL_RESULT -ne $FALSE ]; then
	pr_info "Congratulations! All the package is ready."
	pr_info "Enjoy the ${SDK_NAME}OS!"
	exit 0
else
	pr_warn "The install process is not complete. Please check it!"
	exit $ERR_PKG_UNVAILABLE
fi
