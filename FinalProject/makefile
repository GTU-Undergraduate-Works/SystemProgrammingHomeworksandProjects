all:
	gcc server.c -o server methods.c -lpthread -lm
	gcc server2.c -o server2 methods.c -lpthread -lm
	gcc clients.c -o clients methods.c -lpthread -lm
clean:
	rm server server2 clients