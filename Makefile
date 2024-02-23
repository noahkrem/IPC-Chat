# Note: This file is not finished yet

all: s-talk

s-talk: List.o IPC-Chat.o
	gcc -pthread List.o IPC-Chat.o -o s-talk

List.o: List.c List.h
	gcc -c List.c

IPC-Chat.o: IPC-Chat.c List.h
	gcc -c IPC-Chat.c

clean:
	rm -f s-talk *.o