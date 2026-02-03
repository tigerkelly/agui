# Set export PROFILE=yes to turn on profiling flags.

CC=cc
SRCS=agui.c
OBJS=agui.o

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

$(OBJS): agui.c agui.h

.c.o:
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -rf $(OBJS) $(ARC) test_agui
