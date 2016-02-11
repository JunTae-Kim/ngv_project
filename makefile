CC = g++
CFLAGS = -g -Wall
SRCS = line.cpp
PROG = $(notdir $(CURDIR))

OPENCV = `pkg-config opencv --cflags --libs`
LIBS = $(OPENCV)
.PHONY: all cleans

$(PROG):$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

all: $(PROG)

clean:
	rm -f $(OBJS) $(PROG)
