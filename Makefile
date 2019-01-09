CC=gcc
CFLAGS=-I.
DEPS = dir.h fat32.h FSI.h
OBJ = readfat32.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

readfat32: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
