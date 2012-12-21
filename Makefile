##########################
# Sudoku Solver Makefile #
##########################

# Compiler Flags
CC = gcc
CCFLAGS += -Wall -Werror -Wextra
LDFLAGS =

chip8.o: chip8.c
	$(CC) $(CCFLAGS) chip8.c -o chip8 $(LDFLAGS)

clean:
	rm chip8
