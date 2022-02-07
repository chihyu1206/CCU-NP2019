all: server.o client.o
	gcc server.o -pthread -o server.out
	gcc client.o -pthread -o client.out
server.o: server.c
	gcc -pthread -c server.c 
client.o: client.c
	gcc -pthread -c client.c
clean:
	rm *.o *.out
