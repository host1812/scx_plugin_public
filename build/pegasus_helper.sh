#!/bin/sh 

#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
# Helper script for building Pegasus.
#
# Pegasus has some dependencies that rarely change.  However, if they do
# change, then many Pegasus components must be rebuilt.  Rather than build
# those dependencies needlessly (thus requiring needless rebuilds of Pegasus
# components), this script will intelligently maintain the dependencies
# thus eliminating needless rebuilds.
#
#
# Components that this script provides support for:
#
# ProductVersionFile: ${ROOT}/src/Pegasus/Common/ProductVersion.h
#   Rewrites the file, but restores dates if the new file hasn't changed
#
# ProductDirInclude: ${ROOT}/src/Pegasus/Common/ProductDirectoryStructure.h
#   Rewrites the file, but restores dates if the new file hasn't changed
#
# ConfigDirInclude: ${ROOT}/src/Pegasus/Config/ProductDirectoryStructure.h
#   Rewrites the file, but restores dates if the new file hasn't changed


# Exit on error (rather than continuing)
# (This helps catch issues w/DevBuild on specific platforms)

set -e

# Set up some symbols for ease of using 'cp' and 'cmp'

# For cp:  Rather than --preserve, use -p (works on Suse, RH, Solaris, HP, and AIX)
# For cmp: Rather than --quiet, use -s (works on Suse, RH, Solaris, HP, and AIX)

CP='cp -p'
CMP='cmp -s'


#
# Function Definitions
#

# Function: DoRetentiveBuild
#
# Call Pegasus to build a file, but restore original file if results are
# identical.  The idea here is to avoid dependency issues if files change
# only by timestamp and not by content.
#
# Parameters (Explicit):
#   Build file		File built via the build target
#   Build target	Pegasus build target for the specified file
#
# Parameters (Implicit):
#   PEGASUS_ROOT	Root location for Pegasus tree
#
# On error, this routine will normally abort (generally only due to bad call setup)

DoRetentiveBuild ()
{
    # Algorithm used here:
    # 1. If the file doesn't exist, build it and exit
    # 2. If the file does exist, then:
    #	a) Copy it to intermediate directory (saving date of generated file)
    #	b) Regenerate the file using the Pegasus makefile,
    #	c) Compare the file with our copy.
    #
    # If the new file == saved file, restore saved copy (to restore timestamp).
    # Otherwise, leave new copy alone, forcing rebuilds of Pegasus components.

    GEN_FILE=${1}
    BUILD_TARGET=${2}
    TEMP_FILE=/tmp/pegasus.$$

    # Do basic validation (some errors, like bad build target, won't be caught)
    if [ -z "${GEN_FILE}" -o -z "${BUILD_TARGET}" ]; then
	echo 1>&2 "Function DoRetentiveBuild called with invalid parameters!"
	exit 126
    fi

    if [ -z "${PEGASUS_ROOT}" ]; then
	echo 1>&2 "Function DoRetentiveBuild called without PEGASUS_ROOT set!"
	exit 126
    fi

    # Copy the original file to a temporary location (solely to save attributes if needed)
    [ -f $GEN_FILE ] && ${CP} ${GEN_FILE} ${TEMP_FILE}
    make -C $PEGASUS_ROOT -f Makefile.Release ${BUILD_TARGET}

    if [ ! -f $GEN_FILE -o ! -s $GEN_FILE ]; then
	echo 1>&2 "File \"${GEN_FILE}\" not generated (or empty) by target ${BUILD_TARGET}!"
	exit 126
    fi

    # If we don't have a saved file, we're done
    [ ! -f ${TEMP_FILE} ] && exit 0

    # Compare and restore if needed
    ${CMP} $GEN_FILE $TEMP_FILE
    if [ $? -eq 0 ]; then
	# We generated same file, so restore orginal to save time stamps
	${CP} ${TEMP_FILE} ${GEN_FILE}
    fi
    rm $TEMP_FILE
}

#
# Main code begins
#

if [ $# -ne 2 ]; then
    echo 1>&2 "Usage: $0 <PEGASUS_ROOT dir> <component>"
    exit 127
fi

PEGASUS_ROOT=$1
COMPONENT=$2

# Validate directories

if [ ! -d $PEGASUS_ROOT ]; then
    echo 1>&2 "Invalid Pegasus root directory specified: \"${PEGASUS_ROOT}\""
    exit 127
fi

# Build the component we were asked to build

if (test $COMPONENT = "ProductVersionFile")
then
    # Generated ProductVersion.h file lives at:
    #	$(PEGASUS_ROOT)/src/Pegasus/Common/ProductVersion.h

    DoRetentiveBuild \
	${PEGASUS_ROOT}/src/Pegasus/Common/ProductVersion.h \
	create_ProductVersionFile

elif (test $COMPONENT = "ProductDirInclude")
then
    # Generated ProductDirectoryStructure.h file lives at:
    #	$(PEGASUS_ROOT)/src/Pegasus/Common/ProductDirectoryStructure.h

    DoRetentiveBuild \
	${PEGASUS_ROOT}/src/Pegasus/Common/ProductDirectoryStructure.h \
	create_CommonProductDirectoriesInclude

elif (test $COMPONENT = "ConfigDirInclude")
then
    # Generated ProductDirectoryStructure.h file lives at:
    #	${ROOT}/src/Pegasus/Config/ProductDirectoryStructure.h

    DoRetentiveBuild \
	${PEGASUS_ROOT}/src/Pegasus/Config/ProductDirectoryStructure.h \
	create_ConfigProductDirectoriesInclude

else
    echo 1>&2 "Invalid component specified: \"$COMPONENT\""
    exit 127
fi

exit 0
