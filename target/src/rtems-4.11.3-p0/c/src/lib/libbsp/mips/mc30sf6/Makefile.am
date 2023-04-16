##
## BSP for mct03p board

ACLOCAL_AMFLAGS = -I ../../../../aclocal

include $(top_srcdir)/../../../../automake/compile.am

include_bspdir = $(includedir)/bsp

dist_project_lib_DATA = bsp_specs

include_HEADERS = ../shared/multicore/include/bsp.h
include_HEADERS += ../../shared/include/tm27.h
include_bsp_HEADERS = include/config.h
include_bsp_HEADERS += ../shared/liblnk/regs.h
include_bsp_HEADERS += include/multicore.h

# ISR
include_bsp_HEADERS += ../../shared/include/irq-generic.h
include_bsp_HEADERS += ../../shared/include/irq-info.h
include_bsp_HEADERS += include/irq.h

# DMA
include_bsp_HEADERS += ../shared/multicore/include/dma.h

# Auxilary units
include_bsp_HEADERS += ../shared/multicore/include/auxio.h
include_bsp_HEADERS += ../shared/multicore/include/auxtimer.h
include_bsp_HEADERS += ../shared/multicore/include/auxmath.h

# SpaceWire
include_bsp_HEADERS += ../shared/multicore/include/swic.h
include_bsp_HEADERS += ../mc30sf6/include/swic-regs.h

# CPU cache
include_bsp_HEADERS += ../shared/multicore/include/cpu.h

# MFBSP
include_bsp_HEADERS += ../shared/multicore/include/mfbsp.h
include_bsp_HEADERS += ../shared/multicore/include/lport.h
include_bsp_HEADERS += ../shared/multicore/include/gpio.h

# Flash
include_bsp_HEADERS += include/s29gl.h

nodist_include_HEADERS = include/bspopts.h
nodist_include_bsp_HEADERS = ../../shared/include/bootcard.h
DISTCLEANFILES = include/bspopts.h

nodist_include_HEADERS += ../../shared/include/coverhd.h

noinst_LIBRARIES = libbspstart.a
libbspstart_a_SOURCES = ../shared/multicore/start/start.S ../shared/liblnk/regs.h
project_lib_DATA = start.$(OBJEXT)

dist_project_lib_DATA += startup/linkcmds

noinst_LIBRARIES += libbsp.a
libbsp_a_SOURCES =

# startup
libbsp_a_SOURCES += ../shared/multicore/startup/bspclean.c
libbsp_a_SOURCES += ../../shared/bsplibc.c
libbsp_a_SOURCES += ../../shared/bsppredriverhook.c
libbsp_a_SOURCES += ../../shared/bsppost.c
libbsp_a_SOURCES += ../../shared/bsppretaskinghook.c
libbsp_a_SOURCES += ../../shared/bspgetworkarea.c
libbsp_a_SOURCES += ../shared/multicore/startup/bspstart.c
libbsp_a_SOURCES += ../../shared/bootcard.c
libbsp_a_SOURCES += ../../shared/sbrk.c
libbsp_a_SOURCES += ../shared/multicore/startup/inittlb.c
libbsp_a_SOURCES += ../shared/startup/idttlb.S
libbsp_a_SOURCES += ../shared/multicore/startup/bspreset.c

# clock
libbsp_a_SOURCES += ../shared/clock/clockdrv.c
libbsp_a_SOURCES += ../shared/clock/mips_timer.S
libbsp_a_SOURCES += ../../shared/clockdrv_shell.h

# console
libbsp_a_SOURCES += ../shared/multicore/console/conscfg.c
libbsp_a_SOURCES += ../shared/multicore/console/printk_support.c
libbsp_a_SOURCES += ../../shared/console.c
libbsp_a_SOURCES += ../../shared/console_select.c
libbsp_a_SOURCES += ../../shared/console_read.c
libbsp_a_SOURCES += ../../shared/console_write.c
libbsp_a_SOURCES += ../../shared/console_control.c

# timer
libbsp_a_SOURCES += ../../shared/timerstub.c

libbsp_a_LIBADD  = ../../../libcpu/@RTEMS_CPU@/shared/cache.rel
libbsp_a_LIBADD += ../../../libcpu/@RTEMS_CPU@/shared/interrupts.rel

# isr
libbsp_a_SOURCES += ../../shared/src/irq-generic.c
libbsp_a_SOURCES += ../../shared/src/irq-legacy.c
libbsp_a_SOURCES += ../../shared/src/irq-info.c
libbsp_a_SOURCES += ../../shared/src/irq-shell.c
libbsp_a_SOURCES += ../../shared/src/irq-server.c
libbsp_a_SOURCES += ../shared/multicore/irq/vectorexceptions.c
libbsp_a_SOURCES += ../shared/irq/irq.c
libbsp_a_SOURCES += ../shared/multicore/irq/vectorisrs.c
libbsp_a_SOURCES += ../shared/multicore/irq/interruptmask.c

# DMA
libbsp_a_SOURCES += ../shared/multicore/dma/dma.c

# swic
libbsp_a_SOURCES += ../shared/multicore/spw/swic.c

# CPU cache
libbsp_a_SOURCES += ../shared/multicore/cpu/cpu.c

# MFBSP lport
libbsp_a_SOURCES += ../shared/multicore/mfbsp/lport.c
libbsp_a_SOURCES += ../shared/multicore/mfbsp/gpio.c

# Flash
libbsp_a_SOURCES += flash/s29gl.c

# GDB-stub
gdbstub_CPPFLAGS = -I$(srcdir)/../../mips/shared/gdbstub
noinst_PROGRAMS = gdbstub.rel
gdbstub_rel_SOURCES = ../shared/multicore/gdbstub/mips-stub.c
gdbstub_rel_SOURCES += ../../shared/gdbstub/rtems-stub-glue.c
gdbstub_rel_SOURCES += ../shared/multicore/gdbstub/gdb-support.c
gdbstub_rel_CPPFLAGS = $(AM_CPPFLAGS) $(gdbstub_CPPFLAGS)
gdbstub_rel_LDFLAGS = $(RTEMS_RELLDFLAGS)

libbsp_a_LIBADD += gdbstub.rel

include $(srcdir)/preinstall.am
include $(top_srcdir)/../../../../automake/local.am
