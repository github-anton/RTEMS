#/bin/sh

export RTEMS_HOME=/home/anton/Software/RTEMS

export RTEMS_SRC=${RTEMS_HOME}/target/src/rtems-4.11.2-p0

export PATH=${RTEMS_HOME}/host/4.11/bin:${RTEMS_SRC}:$PATH

export RTEMS_BSP=mct03pem

export RTEMS_MAKEFILE_PATH=${RTEMS_HOME}/host/4.11/mips-rtems4.11/${RTEMS_BSP}
