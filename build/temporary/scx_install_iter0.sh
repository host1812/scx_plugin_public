#!/bin/sh
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
if [ ! `id -u` == 0 ] ; then
    echo "! You must be root to run this script"
    echo ""
    exit 128
fi

SCX_INSTALL_ITER0_BASEDIR=`dirname $0`
SCX_INSTALL_ITER0_TEMPDIR=`mktemp -d`

echo "# pushd $SCX_INSTALL_ITER0_TEMPDIR"
pushd $SCX_INSTALL_ITER0_TEMPDIR
echo "# tar -xzf $SCX_INSTALL_ITER0_BASEDIR/scx_install_iter0.tgz ."
tar -xzf $SCX_INSTALL_ITER0_BASEDIR/scx_install_iter0.tgz .

cd script

pwd

/bin/sh scx_install.sh

