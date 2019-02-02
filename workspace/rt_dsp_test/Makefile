#
# 	Makefile for hello.exe
#

# C source names, if any, go here -- minus the .c
C_PIECES=init
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

# C++ source names, if any, go here -- minus the .cpp
CC_PIECES=
CC_FILES=$(CC_PIECES:%=%.cpp)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES=

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

# Program name minus extension (.exe)
PROGRAM=rtSpaceWireTest

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES) __DSP0__.o

PGMS=${ARCH}/$(PROGRAM).exe

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/leaf.cfg

#
# (OPTIONAL) Add local stuff here using +=
#

DEFINES  +=
CPPFLAGS +=
CFLAGS   +=

#
# CFLAGS_DEBUG_V are used when the `make debug' target is built.
#

LD_PATHS  += 
LD_LIBS   += 
LDFLAGS   +=

#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

CLEAN_ADDITIONS += 
CLOBBER_ADDITIONS +=

all:	${ARCH} $(SRCS) $(PGMS)

# The following links using C rules.
#${PGMS}: ${OBJS} ${LINK_FILES}
#	$(make-exe)
#	(cd o-optimize && $(OBJCOPY) -O binary ${PROGRAM}.exe ${PROGRAM}.bin)

# The following links using C++ rules to get the C++ libraries.
# Be sure you BSP has a make-cxx-exe rule if you use this.
${PGMS}: ${OBJS} ${LINK_FILES}
	$(make-cxx-exe)
	(cd o-optimize && $(OBJCOPY) -O binary ${PROGRAM}.exe ${PROGRAM}.bin)

# Install the program(s), appending _g or _p as appropriate.
# for include files, just use $(INSTALL_CHANGE)
install:  all
	$(INSTALL_VARIANT) -m 555 ${PGMS} ${PROJECT_RELEASE}/bin
