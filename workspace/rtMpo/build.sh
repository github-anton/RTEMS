#!/bin/sh

#. ~/bin/rtems-env.sh 4.11.3 mc30sf6
. ~/bin/rtems-env.sh 4.11.3 mc30mpo2

[ -n "${ProjDirPath}" ] && cd ${ProjDirPath}

make -f rtems.mk $1
