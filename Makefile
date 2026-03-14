# Set export PROFILE=yes to turn on profiling flags.

CC=cc
SRCS=agui.c linkedList.c parse.c qparse.c strqtok.c
OBJS=agui.o linkedList.o parse.o qparse.o strqtok.o

LDFLAGS=-L/usr/local/lib
CFLAGS=-std=gnu99 -g -Wall -I./
ifeq ($(PROFILE),yes)
    CFLAGS += -pg
    LDFLAGS += -pg
endif

ARC=libagui.a

all: $(ARC)

$(ARC): $(OBJS)
	$(AR) -r $(ARC) $(OBJS)

$(OBJS): agui.c agui.h linkedList.h

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(ARC) test_agui
