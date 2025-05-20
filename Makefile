# Project Name
TARGET = realtime-bpm-detection
# TARGET = audio-level-test
# TARGET = usb-print-test

LDFLAGS = -u _printf_float

# Sources
CPP_SOURCES = src/realtime-bpm-detection.cpp
# CPP_SOURCES = src/audio-level-test.cpp
# CPP_SOURCES = src/usb-print-test.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Add BTT Library
C_INCLUDES += -I./lib/BTT
C_SOURCES += lib/BTT/src/BTT.c \
             lib/BTT/src/DFT.c \
             lib/BTT/src/fastsin.c \
             lib/BTT/src/Filter.c \
             lib/BTT/src/Statistics.c \
             lib/BTT/src/STFT.c

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile


# Add optimization flags for size
OPT = -Os
LDFLAGS += -Wl,--gc-sections,--strip-all -Wl,-u,_printf_float