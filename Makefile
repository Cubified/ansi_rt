all: ansi_rt

CC=gcc

LIBS=-lm
CFLAGS=-Os -pipe -s -pedantic
DEBUGCFLAGS=-Og -pipe -g -Wall -Wextra

INPUT=ansi_rt.c
OUTPUT=ansi_rt

RM=/bin/rm

.PHONY: ansi_rt
ansi_rt:
	$(CC) $(INPUT) -o $(OUTPUT) $(LIBS) $(CFLAGS)

debug:
	$(CC) $(INPUT) -o $(OUTPUT) $(LIBS) $(DEBUGCFLAGS)

clean:
	if [ -e $(OUTPUT) ]; then $(RM) $(OUTPUT); fi
