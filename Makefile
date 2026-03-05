# Color Picker Build Script
# -------------------------
# This Makefile automates the build process for the Color Picker application
# on MinGW and Cygwin environments.

# Compiler Configuration
CC = gcc
WINDRES = windres

# Source Files
SRC = colorpicker.c
RC = resource.rc

# Object Files
OBJ = $(SRC:.c=.o)
RES = resource.o

# Target Executable
TARGET = colorpicker.exe

# Compiler Flags
CFLAGS = -Wall -O2
LDFLAGS = -mwindows -lkernel32 -luser32 -lgdi32

# Default Target
.PHONY: all clean

# Build all targets
all: $(TARGET)

# Compile Resource Script
$(RES): $(RC)
	$(WINDRES) $(RC) -o $(RES)

# Compile C Source Code
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Link Executable
$(TARGET): $(OBJ) $(RES)
	$(CC) $(OBJ) $(RES) -o $(TARGET) $(LDFLAGS)

# Clean Build Artifacts
clean:
	rm -f $(OBJ) $(RES) $(TARGET)