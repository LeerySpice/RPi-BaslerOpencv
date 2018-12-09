# Makefile for Basler pylon sample program
.PHONY: all clean

PROJECT = BASLER-EXEC

# The program to build
SRC  = \
    BASLER-EXEC.cpp \
	
OBJS = $(SRC:.cpp=.o)
	
# Installation directories for pylon
PYLON_ROOT ?= /opt/pylon5

# Build tools and flags
LD         := $(CXX)
CPPFLAGS   := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags)
CXXFLAGS   := #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDFLAGS    += $(shell pkg-config --libs --static opencv)
LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs) -lwiringPi

# Rules for building
all: $(OBJS) $(PROJECT)

$(PROJECT): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $< 

clean:
	rm -f $(OBJS)
	rm -f $(PROJECT)
