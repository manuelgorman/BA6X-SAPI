display: server.o ba6x.o
	gcc server.o ba6x.o -o display -g -lm -L"/usr/local/lib" -lHIDAPI

server.o: src/server.c
	gcc -c src/server.c

ba6x.o: src/ba6x.c
	gcc -c src/ba6x.c

clean:
	@rm -f *.o
