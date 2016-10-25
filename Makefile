all: server_TCP_IPv4.c client_TCP_IPv4.c
	g++ -g server_TCP_IPv4.c -o server4
	gcc -g client_TCP_IPv4.c -o client4

clean:
	rm -f server4 client4
	
