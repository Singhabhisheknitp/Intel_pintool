# Specify the target
TARGET_COMPILER = gnu
TARGET = ia32e
APP_TYPE = pintool

# Set PIN_ROOT to your PIN installation directory
PIN_ROOT ?= $(shell pwd)/..

# The source file to compile
SOURCES = matrix_mem_analyzer.cpp

# Include pin build rules
include $(PIN_ROOT)/pin-external-3.31-98869-gfa6f126a8-gcc-linux/source/tools/makefile.rules