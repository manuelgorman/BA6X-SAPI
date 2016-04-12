display: server.o ba6x.o
	gcc server.o ba6x.o -o display -Wall -Wextra -g -lm -L"/usr/local/lib" -lHIDAPI

server.o: src/server.c
	gcc -c src/server.c -Wall -Wextra

ba6x.o: src/ba6x.c
	gcc -c src/ba6x.c -Wall -Wextra

clean:
	@rm -f *.o

install:
	@cp bin/display /usr/local/bin/display
