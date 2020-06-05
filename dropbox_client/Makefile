dropbox_client: main.o ring_buffer.o list.o functions.o
	gcc -o dropbox_client main.o list.o ring_buffer.o functions.o -pthread

list.o:list.c list.h structures.h common.h utils.h
	gcc -c list.c

ring_buffer.o:ring_buffer.c ring_buffer.h
	gcc -c ring_buffer.c

functions.o:functions.c structures.h common.h list.h utils.h
	gcc -c functions.c -pthread

main.o:main.c structures.h common.h ring_buffer.h list.h utils.h functions.o
	gcc -c main.c -pthread

clean:
	rm dropbox_client functions.o main.o ring_buffer.o list.o
