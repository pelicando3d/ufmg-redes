all: server_TCP_IPv4.c client_TCP_IPv4.c
	g++ server_TCP_IPv4.c -o server4
	gcc client_TCP_IPv4.c -o client4

clean:
	rm -f server4 client4
	
