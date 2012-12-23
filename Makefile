##########################
# Sudoku Solver Makefile #
##########################

# Compiler Flags
CC = gcc
CCFLAGS += -Wall -Wextra
LDFLAGS = -framework OpenGL -framework GLUT

all: play

play: chip8.c play.c
	$(CC) $(CCFLAGS) chip8.c play.c -o play $(LDFLAGS)

clean:
	rm play
