TARGET=testhttp_raw

CC=gcc
CFLAGS=-O2
DEPS=err.h
OBJ=testhttp_raw.o err.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

testhttp_raw: $(OBJ)
	$(CC) -o testhttp_raw $(OBJ)
 
.PHONY: clean

clean:
	rm *.o testhttp_raw
