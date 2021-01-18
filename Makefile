compile:
	gcc server.c -std=c99 -Wall -pedantic -pthread -lrt -o viergewinnt_server
	gcc client.c -std=c99 -Wall -pedantic -pthread -lrt -o viergewinnt_client
