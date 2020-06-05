server: main.o list.o functions.o
	gcc -o server main.o list.o functions.o

list.o:list.c list.h structures.h common.h utils.h
	gcc -c list.c

functions.o:functions.c structures.h common.h list.h utils.h
	gcc -c functions.c

main.o:main.c structures.h common.h list.h utils.h functions.o
	gcc -c main.c

clean:
	rm server functions.o main.o list.o
