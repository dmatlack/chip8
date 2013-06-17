# Compiler Flags
CC = gcc
CCFLAGS += -Wall -Wextra
LDFLAGS = -lGL -lglut

all: play

play: chip8.c play.c
	$(CC) $(CCFLAGS) chip8.c play.c -o play $(LDFLAGS)

clean:
	rm -f play
