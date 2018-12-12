all:fork_server.c select_server.c
	gcc fork_server.c -o fork_server
	gcc select_server.c -o select_server
fork_server:fork_server.c
	gcc fork_server.c -o fork_server
select_server:select_server.c
	gcc select_server.c -o select_server
clean:
	rm select_server fork_server
