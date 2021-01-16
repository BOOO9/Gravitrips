compile:
	gcc server.c -std=c99 -Wall -pedantic -pthread -o Server
	gcc client.c -std=c99 -Wall -pedantic -pthread -o Client
