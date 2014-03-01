LIBMODS	:=
MODULES	:=
SRCDIR	:= src $(addprefix src/,$(MODULES))
BINDIR	:= bin
BINMODS	:= $(addprefix bin/,$(MODULES))
TARGET	:= $(BINDIR)/cgExercise05
SRCS	:= $(shell find $(SRCDIR) -maxdepth 1 -name '*.cpp' -o -name '*.c')
HEADERS	:= $(shell find $(SRCDIR) -maxdepth 1 -name '*.h')
OBJECTS := $(subst src,$(BINDIR),$(SRCS:%=%.o))

vpath $(SRCDIR)

CC		= gcc
CXX		= g++
CFLAGS	= -Wall -g
LIBS	=
LDFLAGS	=
DEFS	=
INCLUDES	= $(addprefix -I,$(SRCDIR))

ifeq ($(OS),Windows_NT)
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        CFLAGS += `sdl2-config --cflags`
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CFLAGS += `sdl2-config --cflags`
        LIBS += `sdl2-config --libs`
    endif
    ifeq ($(UNAME_S),Darwin)
        LDFLAGS += -framework OpenGL -framework Cocoa -framework SDL2 -lGLEW -framework GLUT
        LIBS += -I/System/Library/Frameworks/SDL2.framework/Headers -I/System/Library/Frameworks/GLUT.framework/Headers
    endif
endif

.PHONY: all clean

all: $(BINDIR) $(BINMODS) $(TARGET)
	@echo "copying shader files..."
	@cp shader05* $(BINDIR)

$(BINMODS):
	mkdir -p $@

$(BINDIR):
	mkdir -p $@

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BINDIR)/%.cpp.o: src/%.cpp
	$(CXX) $(LIBS) $(CFLAGS) $(INCLUDES) $(DEFS) -c $< -o $@

$(BINDIR)/%.c.o: src/%.c
	$(CC) $(LIBS) $(CFLAGS) $(INCLUDES) $(DEFS) -c $< -o $@

$(SRCS): $(HEADERS)

clean:
	-rm -f $(BINDIR)/*~ $(addsuffix /*.o,$(BINMODS)) $(BINDIR)/*.o $(TARGET)

distclean:: clean
