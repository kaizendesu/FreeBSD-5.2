#!/bin/sh
#
# $FreeBSD$
#

# PROVIDE: early
# REQUIRE: disks localswap
# BEFORE:  fsck
# KEYWORD: FreeBSD

#
# Support for legacy /etc/rc.early script
#
if [ -r /etc/rc.early ]; then
	. /etc/rc.early
fi
