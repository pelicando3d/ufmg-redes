all: server_TCP_IPv6.cpp client_TCP_IPv6.cpp
	g++ -g -Wall server_TCP_IPv6.cpp -o servidorFTP -lpthread
	g++ -g -Wall client_TCP_IPv6.cpp -o clienteFTP

clean:
	rm -f servidorFTP clienteFTP
	
