all: server_TCP_IPv4.c client_TCP_IPv4.c
	g++ -g server_TCP_IPv6.cpp -o server
	g++ -g client_TCP_IPv6.c -o client

clean:
	rm -f server client
	
