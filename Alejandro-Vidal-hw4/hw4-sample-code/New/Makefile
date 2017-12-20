libmem.so: mem.o
	gcc -shared -o libmem.so mem.o
mem.o: mem.c mem.h
	gcc -c -fpic -o mem.o mem.c


