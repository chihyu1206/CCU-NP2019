all: server.c client.c
	gcc server.c -pthread -o server
	gcc client.c -pthread -o client
server: server.c
	gcc server.c -pthread -o server
client: client.c
	gcc client.c -pthread -o client
clear:
	rm server
	rm client
