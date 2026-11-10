.DEFAULT_GOAL := build

CHECK_TASKS := lint build
BUILD_OUTPUTS := pavolume
CONTAINER_BUILD_ARGS := PYTHON_VERSION=$(shell cat .python-version)

override COMMAND := $(value COMMAND)
override TEST_ARGS := $(value TEST_ARGS)

export CHECK_TASKS BUILD_OUTPUTS CONTAINER_BUILD_ARGS TEST_ARGS COMMAND

.PHONY: check check-info lint test build exec

check lint test build exec:
	@./tests/container/run $@

check-info:
	@./tests/container/run info


CC      ?= gcc
CFLAGS  += -std=c99 -pedantic -Wall -Wextra -Wno-discarded-qualifiers -I$(PREFIX)/include
CFLAGS  += -D_POSIX_C_SOURCE=200112L
LDFLAGS += -L$(PREFIX)/lib

LIBS     = -lm -lpulse
TARGET   = pavolume

PREFIX    ?= /usr/local
BINPREFIX  = $(PREFIX)/bin

default all: build

debug:
	@$(MAKE) exec COMMAND='make debug-native'

debug-native: CFLAGS += -O0 -g
debug-native: $(TARGET)

build-native: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $@.c $(LIBS)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -p $(TARGET) "$(DESTDIR)$(BINPREFIX)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)/$(TARGET)"

clean:
	rm -f $(TARGET)

.PHONY: all debug debug-native build-native default install uninstall clean
