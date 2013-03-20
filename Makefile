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

INCLUDES = -I/opt/local/include/ni -I/opt/local/include/opencv
LIBS_PATH = -L/opt/local/lib/
LIBS = -lOpenNI -lboost_program_options-mt -lopencv_video -lopencv_highgui -lopencv_core -lopencv_imgproc

CC = g++
CCFLAGS = -O2
LINK = g++
SRC = oni2src.cpp
OBJ = oni2avi.o

$(EXE):	$(OBJ)
	$(LINK) -o $(EXE) $(OBJ) $(LIBS_PATH) $(LIBS)

%.o:%.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c $<

clean:
	rm -fr $(ROOT) *.o *.d
