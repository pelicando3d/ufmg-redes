all: impaciente clienteUDP servidorUDP

clean: 
	@rm -f *.o impaciente cliente

impaciente: impaciente.c 
	gcc $(CFLAGS) -o $@ impaciente.c

tp_socket.o: tp_socket.c tp_socket.h
	gcc $(CFLAGS) -c $< -o $@

clienteUDP: tp_socket.o

servidorUDP: tp_socket.o