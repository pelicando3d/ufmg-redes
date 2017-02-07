all: client server

client: client.o tp_socket.o
	gcc -g client.o tp_socket.o -o c.app

server: server.o tp_socket.o
	gcc -g server.o tp_socket.o -o s.app


tp_socket.o: tp_socket.c tp_socket.h
	gcc -g -c tp_socket.c

client.o: client.c
	gcc -g -c client.c -Wall


server.o: server.c
	gcc -g -c server.c

clean:
	rm -f *.app *.o
	
