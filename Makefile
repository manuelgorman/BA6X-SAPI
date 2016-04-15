display: server.o ba6x.o
	gcc server.o ba6x.o -o display -Wpointer-sign -g -lm -L"/usr/local/lib" -lhidapi-libusb -lpthread

server.o: src/server.c
	gcc -c src/server.c -Wpointer-sign

ba6x.o: src/ba6x.c
	gcc -c src/ba6x.c -Wpointer-sign

clean:
	@rm -f *.o

install:
	@cp bin/display /usr/local/bin/display
