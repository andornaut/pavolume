CC      ?= gcc
CFLAGS  += -std=c99 -pedantic -Wall -Wextra -Wno-discarded-qualifiers -I$(PREFIX)/include
CFLAGS  += -D_POSIX_C_SOURCE=200112L
LDFLAGS += -L$(PREFIX)/lib

LIBS     = -lm -lpulse
TARGET   = pavolume

PREFIX    ?= /usr/local
BINPREFIX  = $(PREFIX)/bin

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

all: $(TARGET)

debug: CFLAGS += -O0 -g
debug: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $@.c $(LIBS)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -p $(TARGET) "$(DESTDIR)$(BINPREFIX)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)/$(TARGET)"

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all debug default install uninstall clean
.PRECIOUS: $(TARGET) $(OBJECTS)
