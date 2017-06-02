# (C) Copyright Kirill Lykov 2013.
#
# Distributed under the FreeBSD Software License (See accompanying file license.txt) 
#
# Make file for oni2avi. Check include and lib pathes before runing.

SHELL = /bin/sh
.SUFFIXES: .cpp .u
.IGNORE:

ROOT =  oni2avi
EXE =   $(ROOT)


UNAME := $(shell uname)
CC = clang++

ifeq ($(UNAME), Linux)
    INCLUDES = -I/usr/local/include/ni2 -I/usr/local/include/opencv
    LIBS_PATH = -L/usr/local/lib/
endif
ifeq ($(UNAME), Darwin)
    INCLUDES = -I/usr/local/include/ -I/usr/local/include/ni2/ -I/usr/local/opt/opencv3/include
    LIBS_PATH = -L/usr/local/lib/ni2 -L/usr/local/lib/ -L/usr/local/opt/opencv3/lib
	#if default gcc, try to use clang
	#GCCVERSION = $(shell gcc --version | grep ^gcc | sed 's/^.* //g')
	#ifeq ("$(GCCVERSION)", "4.2.1")
	#	CC = clang
	#endif
endif

LIBS = -lOpenNI2 -lboost_system-mt -lboost_program_options-mt -lboost_filesystem-mt -lopencv_video -lopencv_highgui -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs

CCFLAGS = -std=c++11 -O0 -g
LINK = clang++
SRC = oni2src.cpp
OBJ = oni2avi.o

$(EXE):	$(OBJ)
	$(LINK) -o $(EXE) $(OBJ) $(LIBS_PATH) $(LIBS)

%.o:%.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c $<

clean:
	rm -fr $(ROOT) *.o *.d *.dSYM
