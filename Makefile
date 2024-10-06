#Compiler and Linker
CC          := gcc

#The Target Binary Program
TARGET      := p2pchat

#The Directories, Source, Includes, Objects, Binary and Resources
SRCDIR      := src
BUILDDIR    := build
TARGETDIR   := bin
RESDIR      := res
SRCEXT      := c
OBJEXT      := o

LINKER_FLAGS := -lm
COMPILER_FLAGS := 

#Flags, Libraries and Includes
CFLAGS      := -fopenmp -Wall -O3 -g
LIB         := -fopenmp -lm -larmadillo
INC         := -I$(INCDIR) -I/usr/local/include
INCDEP      := -I$(INCDIR)

BASE_PATH := $(shell pwd)
DEBUG := #-fsanitize=address -g


SOURCES     := $(shell find $(BASE_PATH) -name '*.c')
OBJECTS     := $(SOURCES:$(BASE_PATH)/%.c=$(BASE_PATH)/$(BUILDDIR)/%.o)
HEADERS 	:= $(shell find $(BASE_PATH) -name '*.h')

all: directories $(OBJECTS) compile

$(BASE_PATH)/$(BUILDDIR)/%.o: $(BASE_PATH)/%.c $(HEADERS)
	$(CC) -c $(DEBUG) -I $(BASE_PATH) $(COMPILER_FLAGS) $< -o $@

directories:
	@mkdir -p $(BUILDDIR)/src
	@mkdir -p $(TARGETDIR)

compile:
	$(CC) $(DEBUG) $(OBJECTS) $(LINKER_FLAGS) -o $(TARGETDIR)/$(TARGET)

rebuild:
	+$(MAKE) clean
	+$(MAKE) all

run: all
	-./$(TARGETDIR)/$(TARGET)

.PHONY: clean directories compile rebuild run
clean:
	-rm -rf $(BUILDDIR)
	-rm -rf $(TARGETDIR)

## Copied (and then modified) from stackoverflow: https://stackoverflow.com/a/27794283