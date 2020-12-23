/* server.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUF 1024


void setToken(char board[6][7], char field[])
{
	int row = 0;

	int set= atoi(field);
	set--;

	while(board[row][set] == ' ')row++;

	board[row-1][set] = 'X';
}



int main (int argc, char **argv) {

 char gameBoard [6][7];

	for(int i=0; i < 6; i++) {
    		  for(int j=0; j < 7; j++) {
        		gameBoard [i][j] =' ';
     		 }
  	 }


  int create_socket, new_socket;
  socklen_t addrlen;
  char *buffer = malloc (BUF);
  ssize_t size;
  struct sockaddr_in address;
  const int y = 1;

  int* setField;
//  printf ("\e[2J");
if( argc < 2 ){
     printf("Usage: %s Portnmbr\n", *argv);
     exit(EXIT_FAILURE);
   }



  if ((create_socket=socket (AF_INET, SOCK_STREAM, 0)) > 0)
    printf ("socket create succes\n");
  setsockopt( create_socket, SOL_SOCKET,
              SO_REUSEADDR, &y, sizeof(int));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = atoi(argv[1]);
  if (bind ( create_socket,
             (struct sockaddr *) &address,
             sizeof (address)) != 0) {
    printf( "Der Port ist nicht frei – belegt!\n");
  }
  listen (create_socket, 5);
  addrlen = sizeof (struct sockaddr_in);
  while (1) {
     new_socket = accept ( create_socket,
                           (struct sockaddr *) &address,
                           &addrlen );
     if (new_socket > 0)
      printf ("Ein Client (%s) ist verbunden ...\n",
         inet_ntoa (address.sin_addr));
     do {
//        printf ("Nachricht zum Versenden: ");
//        fgets (buffer, BUF, stdin);
        send (new_socket, gameBoard, sizeof (gameBoard), 0);
size =        recv (new_socket, buffer, BUF-1, 0);
        if( size > 0)
           buffer[size] = '\0';
        printf ("Nachricht empfangen: %s\n", buffer);

	setToken(gameBoard, buffer);

     } while (strcmp (buffer, "quit\n") != 0);
     close (new_socket);
  }
  close (create_socket);
  return EXIT_SUCCESS;
}
