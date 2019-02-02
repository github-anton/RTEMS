#ifndef SPACEWIRE_SPEED_TEST_H_
#define SPACEWIRE_SPEED_TEST_H_

#include "CMcSpaceWire.h"

#define WRITER_DEV_PATH		"/dev/spw0"
#define READER_DEV_PATH		"/dev/spw1"

#define SPACEWIRE_WRITER_CONSTRUCTOR   CMcSpaceWire(WRITER_DEV_PATH, 4096, 0)
#define SPACEWIRE_READER_CONSTRUCTOR   CMcSpaceWire(READER_DEV_PATH, 4096, 0) ;

#if 1
#   define ENABLE_WRITER
#endif

#if 0
#   define ENABLE_READER
#endif

#define WRITER_RATE       200      // MHz

#endif
