compile:
	gcc server.c -pthread -o Server
	gcc client.c -pthread -o Client
	gcc gravitrips.c -o Gravitrips
