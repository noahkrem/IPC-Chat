# Note: This file is not finished yet

all: test

test: list.o testDriver.o
	gcc list.o testDriver.o -o test

list.o: list.c list.h
	gcc -c list.c

testDriver.o: testDriver.c list.h
	gcc -c testDriver.c

clean:
	rm -f test *.o