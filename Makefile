TARGET: testhttp_raw test

CC=gcc
CFLAGS= -Wall -Wextra
DEPS=err.h connect.h http.h
OBJ=err.o connect.o http.o

debug: CFLAGS += -g
debug: testhttp_raw test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

testhttp_raw: $(OBJ) testhttp_raw.o
	$(CC) -o testhttp_raw $^

test: $(OBJ) test.o
	$(CC) -o test $^
 
.PHONY: clean

clean:
	rm *.o testhttp_raw test
