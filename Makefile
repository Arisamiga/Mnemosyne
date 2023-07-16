

# Compiler settings
CC	= vc
LINK = vc

# CFAGS and Linker Settings
CFLAGS = +aos68k -c99
LFLAGS = +aos68k -lauto -lamiga


# INCLUDES settings
INCLUDES = -I$(NDK32_INC)

# Object files
OBJ = src/funcs.o src/main.o src/scan.o src/window.o src/aboutWin.o

# Build command for .o -> Executable
Mnemosyne: $(OBJ)
	@$(LINK) $(OBJ) $(LFLAGS) -o $@

# Compiler for .c -> .o
%.o: %.c
	@echo "\nCompiling $<"
	$(CC) -c $< -o $*.o $(CFLAGS) $(INCLUDES)

# Files and Dependances
src/funcs.o: src/funcs.h src/funcs.c
 
src/main.o:	src/main.c	src/scan.h src/window.h

src/scan.o:	src/scan.h	src/scan.c src/funcs.h

src/window.o: src/window.h src/window.c src/funcs.h src/aboutWin.h

src/aboutWin.o: src/aboutWin.h src/aboutWin.c src/funcs.h src/window.h

# Clean command for .o files and current Executable
clean:
	@rm src/*.o Mnemosyne
