# Project Name
TARGET = realtime-bpm-detection

# Sources
CPP_SOURCES = src/realtime-bpm-detection.cpp

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
