CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
CFLAGS+=-I./plugin-sdk
LDFLAGS=-lprs

SRCDIR=$(shell basename $(shell pwd))
VERSION=1.0
TARNAME=$(SRCDIR)-$(VERSION)

SOURCE1=cmd.c plugin-sdk/parse.c plugin-sdk/plugin.c main.c
OBJECT1=$(SOURCE1:%.c=%.c.o)
TARGET1=netcom

OBJECTS=$(OBJECT1)
TARGETS=$(TARGET1)

.PHONY: all clean distclean dist
all: $(TARGETS)
	@echo "Building all plugins..."
	@cd plugin-sdk && $(MAKE) all

clean:
	@echo -n "Cleaning project... "
	@rm -f $(OBJECTS) $(TARGETS) && echo "done!" || echo "failed!"
	@cd plugin-sdk && $(MAKE) clean

dist: distclean
	@echo "Building distribution: $(TARNAME)..."
	@(cd .. && tar cv --exclude=.git ./$(SRCDIR) | xz -9 > $(TARNAME).tar.xz) && echo "Finished building distribution." || echo "Failed to build distribution."

distclean:
	@echo "Cleaning distribution... "
	@$(MAKE) clean
	@rm -f *.bak && echo "done!" || echo "failed!"

$(TARGET1): $(OBJECT1)
	@echo -n "Building project: $(TARGET1) "
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) && echo "- [DONE]" || echo "- [FAIL]"

%.c.o: %.c
	@echo "Compiling source file: $< => $@"
	@$(CC) $(CFLAGS) -c $< -o $@

