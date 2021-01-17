compile:
	gcc server.c -std=c99 -Wall -pedantic -pthread -lrt -o Server
	gcc client.c -std=c99 -Wall -pedantic -pthread -lrt -o Client
