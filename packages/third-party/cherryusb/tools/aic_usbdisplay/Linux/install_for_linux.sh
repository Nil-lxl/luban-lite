#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2024 ArtInChip Technology Co., Ltd

MY_NAME=$0
TOPDIR=$PWD
AIC_UD_PKG="AicUSBDisplay"

# Return value
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
	echo
	echo -e "${COLOR_WHITE}>>> $*${COLOR_END}"
}

check_root()
{
	CUR_USER=$(whoami)
	if [ "$CUR_USER" = "root" ]; then
		return
	fi
	sudo -l -U $(whoami) | grep ALL > /dev/null
	if [ $? -eq 0 ]; then
		return
	fi

	pr_warn $MY_NAME "must install package with 'sudo'. "
	pr_warn "Your passward will be safe and always used locally."
}

check_os()
{
	if [ -f /etc/lsb-release ]; then
		OS_VER=$(cat /etc/lsb-release | grep RELEASE | awk -F '=' '{print $2}')
		OS_TYPE="Ubuntu"
	elif [ -f /etc/issue ]; then
		cat /etc/issue 2>&1 | grep Debian > /dev/null
		if [ $? -eq 0 ]; then
			OS_VER=$(cat /etc/issue | awk '{print $3}')
			OS_TYPE="Debian"
		fi
	else
		pr_err "Unknow system OS"
		exit $ERR_UNSUPPORTED
	fi
}

check_arch()
{
	uname -m | grep aarch64 > /dev/null
	if [ $? -eq 0 ]; then
		ARCH_TYPE='arm64'
		return
	fi

	uname -m | grep x86_64 > /dev/null
	if [ $? -eq 0 ]; then
		ARCH_TYPE='amd64'
		return
	fi

	pr_err "Unsupported CPU architecture: "
	exit $ERR_UNSUPPORTED
}

check_pi()
{
	uname -r | grep "rpi" > /dev/null
	if [ $? -eq 0 ]; then
		echo It is Raspberry PI
		IS_RPI=YES
		XORG_CFG_WAY="sudo raspi-config\n\t-> 6 Advanced Options\n\t\t-> A6 X11/Xwayland\n"
	else
		XORG_CFG_WAY="sudo vim /etc/gdm3/custom.conf\n\tthen unncomment \"#WaylandEnable=false\"\n"
	fi
}

check_xorg()
{
	ps -ef | grep Xorg | grep -v grep > /dev/null
	if [ $? -ne 0 ]; then
		pr_err "Only support Xorg server as to now!"
		pr_info "Try switch to Xorg in this way:"
		echo ------------------------------------------------
		printf "$XORG_CFG_WAY"
		echo ------------------------------------------------
		exit $ERR_UNSUPPORTED
	fi
}

check_root
check_os
check_arch
check_pi
check_xorg

pr_info "Current system is $OS_TYPE $OS_VER $ARCH_TYPE"

echo
PKG_NAME=$(find . -name "${AIC_UD_PKG}*${ARCH_TYPE}*.deb" | xargs ls -t -1 | head -1)
if [ "x$PKG_NAME" = "x" ]; then
	pr_warn "Can not find $AIC_UD_PKG DEB package for current system"
	exit $ERR_PKG_UNVAILABLE
else
	echo Found $PKG_NAME
fi

if [ "$OS_TYPE" = "Debian" ] && [ "$OS_VER" -lt "12" ] && [ "x$IS_RPI" = "xYES" ]; then
	pr_info Check Xorg ...
	XORG_PKG_NAME=$(find . -name "xserver-xorg-core_1.20.11*.deb" | xargs ls -t -1 | head -1)
	if [ "x$XORG_PKG_NAME" = "x" ]; then
		pr_warn "Can not find Xorg package for $OS_TYPE $OS_VER"
		exit $ERR_PKG_UNVAILABLE
	else
		echo Install $XORG_PKG_NAME first ...
	fi

	echo
	sudo dpkg -i $XORG_PKG_NAME
	if [ $? -ne 0 ]; then
		pr_err "Failed to install $XORG_PKG_NAME"
		exit $ERR_CANCEL
	fi
fi

pr_info Check dkms ...
sudo apt install dkms
if [ $? -ne 0 ]; then
	pr_err "Failed to install dkms"
	pr_err "Maybe the software source is not accessable! Please check it"
	exit $ERR_NET_UNVAILABLE
fi

echo
dpkg -s $AIC_UD_PKG | grep "Status" | grep "installed" > /dev/null
if [ $? -eq 0 ]; then
	pr_info "$AIC_UD_PKG was already installed, so remove it first ..."
	sudo dpkg -r $AIC_UD_PKG
fi

pr_info Install $PKG_NAME ...
sudo dpkg -i $PKG_NAME
if [ $? -ne 0 ]; then
	pr_err "Failed to install $PKG_NAME"
	exit $ERR_CANCEL
fi

pr_info $AIC_UD_PKG installed successfully!
