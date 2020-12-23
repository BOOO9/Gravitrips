/* client.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUF 1024


void printBoard(char board[6][7])
{
	printf("\e[1;1H\e[2J");

	printf("  1 2 3 4 5 6 7\n"); 
	printf(" ---------------\n");

	for(int i=0; i < 6; i++) {
		printf("%d", i+1);
    		  for(int j=0; j < 7; j++) {
        		 printf("|%c", board[i][j]);
		   }
		printf("|\n");
	}

	printf(" ---------------\n\n");
}


int main (int argc, char **argv) {
  int create_socket;
  char *buffer = malloc (BUF);
  struct sockaddr_in address;
  int size;

  int *entry;

  char gameBoard [6][7];

  if( argc < 3 ){
     printf("Usage: %s ServerAdresse  Portnmbr\n", *argv);
     exit(EXIT_FAILURE);
  }
//  printf ("\e[2J");
  if ((create_socket=socket (AF_INET, SOCK_STREAM, 0)) > 0)
    printf ("Socket wurde angelegt\n");
  address.sin_family = AF_INET;
  address.sin_port =atoi(argv[2]);
  inet_aton (argv[1], &address.sin_addr);
  if (connect ( create_socket,
                (struct sockaddr *) &address,
                sizeof (address)) == 0)
    printf ("Verbindung mit dem Server (%s) hergestellt\n",
       inet_ntoa (address.sin_addr));
  do {
      size = recv(create_socket, gameBoard, sizeof(gameBoard), 0);
      if( size > 0)
         buffer[size] = '\0';
//      printf ("Nachricht erhalten: %s\n", buffer);

	printBoard(gameBoard);

    if (strcmp (buffer, "quit\n")) {
         printf ("select column: ");
         fgets(buffer, BUF, stdin);

//	printf("%s", entry);

         send(create_socket, buffer, strlen(buffer), 0);
       }
  } while (strcmp (buffer, "quit\n") != 0);
  close (create_socket);
  return EXIT_SUCCESS;
}
