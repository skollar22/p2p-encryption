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

#Flags, Libraries and Includes
CFLAGS      := -fopenmp -Wall -O3 -g
LIB         := -fopenmp -lm -larmadillo
INC         := -I$(INCDIR) -I/usr/local/include
INCDEP      := -I$(INCDIR)

BASE_PATH := $(shell pwd)
DEBUG := #-fsanitize=address -g

#---------------------------------------------------------------------------------
#DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------
SOURCES     := $(shell find $(BASE_PATH) -name '*.c')
OBJECTS     := $(SOURCES:$(BASE_PATH)/%.c=$(BASE_PATH)/$(BUILDDIR)/%.o)
HEADERS 	:= $(shell find $(BASE_PATH) -name '*.h')

all: directories $(OBJECTS) compile
# @mkdir -p build
# $(CC) $(DEBUG) $(OBJECTS) $(LINKER_FLAGS) -o $(BUILDDIR)/$(TARGET)

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


#############################################################################################################
####################################### copied from stackoverflow ###########################################
################################## https://stackoverflow.com/a/27794283 #####################################
#############################################################################################################


# #Compiler and Linker
# CC          := g++

# #The Target Binary Program
# TARGET      := game

# #The Directories, Source, Includes, Objects, Binary and Resources
# SRCDIR      := src
# INCDIR      := inc
# BUILDDIR    := build
# TARGETDIR   := bin
# RESDIR      := res
# SRCEXT      := cpp
# DEPEXT      := d
# OBJEXT      := o

# #Flags, Libraries and Includes
# CFLAGS      := -fopenmp -Wall -O3 -g
# LIB         := -fopenmp -lm -larmadillo
# INC         := -I$(INCDIR) -I/usr/local/include
# INCDEP      := -I$(INCDIR)

# #---------------------------------------------------------------------------------
# #DO NOT EDIT BELOW THIS LINE
# #---------------------------------------------------------------------------------
# SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
# OBJECTS     := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

# #Defauilt Make
# all: resources $(TARGET)

# #Remake
# remake: cleaner all

# #Copy Resources from Resources Directory to Target Directory
# resources: directories
#     @cp $(RESDIR)/* $(TARGETDIR)/

# #Make the Directories
# directories:
#     @mkdir -p $(TARGETDIR)
#     @mkdir -p $(BUILDDIR)

# #Clean only Objecst
# clean:
#     @$(RM) -rf $(BUILDDIR)

# #Full Clean, Objects and Binaries
# cleaner: clean
#     @$(RM) -rf $(TARGETDIR)

# #Pull in dependency info for *existing* .o files
# -include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

# #Link
# $(TARGET): $(OBJECTS)
#     $(CC) -o $(TARGETDIR)/$(TARGET) $^ $(LIB)

# #Compile
# $(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
#     @mkdir -p $(dir $@)
#     $(CC) $(CFLAGS) $(INC) -c -o $@ $<
#     @$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
#     @cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
#     @sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
#     @sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
#     @rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

# #Non-File Targets
# .PHONY: all remake clean cleaner resources