CC=gcc
CFLAGS=-I.
DEPS = dir.h fat32.h FSI.h
OBJ = a4q1.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

a4q1: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)