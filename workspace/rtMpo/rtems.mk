#
# 	Makefile for hello.exe
#

# C source names, if any, go here -- minus the .c
C_PIECES=src/init src/FilesystemImage src/Shared/crc16 src/Shared/auxio
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

# C++ source names, if any, go here -- minus the .cpp
#CC_PIECES=Init CMachine CRingBuf CMpoMachine CSwReceiver CSwReceiver_test
CC_PIECES=\
	src/Shared/CRingBuffer src/Shared/CTask src/Shared/CFiniteMachine \
	src/Shared/CPushStack src/Shared/CStack src/Shared/CBuffer \
	src/Shared/CWndBuffer src/Shared/CFPGAPacketParser \
	src/Shared/CMMIComposer src/Shared/CFPGA src/Shared/CFPGA_Default \
	\
	src/System/CARINCSched src/System/CMpoSched \
	\
	src/Tasks/CSysMpoMachine src/Tasks/CSysSwIO \
	src/Tasks/CSysInit src/Tasks/CAppTransmit \
	src/Tasks/CAppService \
	\
	src/Mpo
#CC_PIECES=Init CSpaceWire CMcSpaceWire SpaceWire_speedTest
CC_FILES=$(CC_PIECES:%=%.cpp)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES=

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

# Program name minus extension (.exe)
PROGRAM=rtMpo

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)

PGMS=${ARCH}/$(PROGRAM).exe

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/leaf.cfg

#
# (OPTIONAL) Add local stuff here using +=
#

DEFINES  +=
CPPFLAGS += -I$(RTEMS_HOME)/workspace/libSpaceWire/include -I./src/Shared \
	-I$(RTEMS_HOME)/workspace/libMfbsp/include
CFLAGS   +=

#
# CFLAGS_DEBUG_V are used when the `make debug' target is built.
#

LD_PATHS  += 
LD_LIBS   += -L$(RTEMS_HOME)/workspace/libSpaceWire/o-optimize -lSpaceWire \
	-L$(RTEMS_HOME)/workspace/libMfbsp/o-optimize -lMfbsp
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

${ARCH}/%.o: %.c
	mkdir -p `dirname $@` 2>/dev/null ; true
	${COMPILE.c} $(AM_CPPFLAGS) $(AM_CXXFLAGS) -o $@ $<

${ARCH}/%.o: %.cpp
	mkdir -p `dirname $@` 2>/dev/null ; true
	${COMPILE.cc} -std=c++11 $(AM_CPPFLAGS) $(AM_CXXFLAGS) -o $@ $<

#
# Build In-Memory filesystem image which contains
# test files with AIS signals inside PLD packets.
#
$(SRCS): stamp-fs-image

$(ARCH)/FilesystemImage.o: stamp-fs-image src/FilesystemImage.c src/FilesystemImage.h

FilesystemImage: $(ARCH) rootfs
	cd rootfs ; \
	tar cf ../FilesystemImage --exclude CVS --exclude .cvsignore .

stamp-fs-image: $(ARCH) FilesystemImage
	$(PROJECT_ROOT)/bin/rtems-bin2c FilesystemImage FilesystemImage
	mv FilesystemImage.* src
	ls -l src/Filesystem*
	touch stamp-fs-image

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

clean:
	rm -f a.out core mon.out gmon.out
	rm -rf 
	rm -rf  a.out *.o *.BAK Depends-o-optimize.tmp
	rm -rf o-optimize o-debug
	rm -f src/FilesystemImage.*
	
