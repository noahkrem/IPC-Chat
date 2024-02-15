# Note: This file is not finished yet

all: s-talk

test: list.o IPC-Chat.o
	gcc list.o IPC-Chat.o -o s-talk

list.o: list.c list.h
	gcc -c list.c

IPC-Chat.o: IPC-Chat.c list.h
	gcc -c IPC-Chat.c

clean:
	rm -f s-talk *.o