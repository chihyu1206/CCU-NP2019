all: fork_server.o select_server.o
	gcc fork_server.o -o fork_server.out
	gcc select_server.o -o select_server.out
fork_server.o: fork_server.c
	gcc -c fork_server.c
select_server.o:select_server.c
	gcc -c select_server.c
clean:
	rm *.o *.out
