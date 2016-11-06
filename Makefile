all: server_TCP_IPv6.cpp client_TCP_IPv6.cpp
	g++ -g server_TCP_IPv6.cpp -o server -lpthread
	g++ -g client_TCP_IPv6.cpp -o client

clean:
	rm -f server client
	
