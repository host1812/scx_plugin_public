#!/bin/sh
#
# Post-installation helper functions
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
#   Date: 2007-05-30 17:00
#
#


######################################################################
# Create directory if not already exist
#
create_dir() {
    if [ -n "$2" ]; then
	if [ ! -d $1/$2 ]; then
	    mkdir $1/$2 2>/dev/null
	fi
    fi
}


######################################################################
# Identify Operating system
#
os_identification() {
    
    OS_NAME=(unknown)
    OS_DISTRIBUTION=(unknown)
    OS_KERNELVERSION=(unknown)
    
    if [ "`uname`" = "Linux" ]; then
	OS_NAME=Linux
	OS_KERNELVERSION=`uname -r`
	
        # Identify distribution
	if [ "`cat /proc/version | grep -o 'SUSE Linux'`" = "SUSE Linux" ]; then
	    OS_DISTRIBUTION=SUSE
	fi
    fi
}

######################################################################
# Identify Host and domain name
#
# $1 = OS Name
# $2 = OS Distribution
#
hostname_identification() {
    
    HOST_NAME=(unknown)
    HOST_FQDN=(unknown)
    
    if [ "$1" = "Linux" ] && [ "$2" = "SUSE" ]; then
        HOST_FQDN=`head -1 /etc/HOSTNAME 2> /dev/null`
	
	if [ "$HOST_FQDN" = "" ]; then
	    HOST_FQDN=localhost.localdomain
	fi
	
	NUMBEROFNAMEPARTS=`echo $HOST_FQDN | grep -o -E "[-a-z0-9]+" | wc -l`
	if [ $NUMBEROFNAMEPARTS = 1 ]; then
            # We have only the hostname specified
	    HOST_NAME=$HOST_FQDN
        else
	    # Get the first name in the FQDN string
	    HOST_NAME=`echo $HOST_FQDN | grep -o -E "[-a-z0-9]+" | head -n 1`
        fi
    fi
}


