CC      ?= gcc
CFLAGS  += -std=c99 -pedantic -Wall -Wextra -Wno-discarded-qualifiers -I$(PREFIX)/include
CFLAGS  += -D_POSIX_C_SOURCE=200112L
LDFLAGS += -L$(PREFIX)/lib

LIBS     = -lm -lpulse
TARGET   = pavolume

PREFIX    ?= /usr/local
BINPREFIX  = $(PREFIX)/bin

default: $(TARGET)
all: $(TARGET)

debug: CFLAGS += -O0 -g
debug: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $@.c $(LIBS)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -p $(TARGET) "$(DESTDIR)$(BINPREFIX)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)/$(TARGET)"

clean:
	rm -f $(TARGET)

.PHONY: all debug default install uninstall clean

PYTHON_IMAGE := python:$(shell cat .python-version)
MARKDOWN_IMAGE := node:24.22.2
SHELLCHECK_IMAGE := koalaman/shellcheck:v0.11.0
CONTAINER_BUILD_ARGS := PYTHON_VERSION=$(shell cat .python-version)

export PYTHON_IMAGE MARKDOWN_IMAGE SHELLCHECK_IMAGE CONTAINER_BUILD_ARGS
export CHECK_TIMEOUT_SECONDS CHECK_MEMORY CHECK_SCRATCH_SIZE CHECK_WORK_SIZE CHECK_TMP_SIZE CONTAINER_IMAGE

.PHONY: check lint check-build

check lint:
	@./tests/container/check-stock $@

check-build:
	@CHECK_TASKS=build ./tests/container/run check
