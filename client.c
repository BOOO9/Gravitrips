/* client.c */
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define BUF 1024

int run = 1;

char *progname;

void printBoard(char board[6][7]) {
  printf("\e[1;1H\e[2J");

  printf("  1 2 3 4 5 6 7\n");
  printf(" ---------------\n");

  for (int i = 0; i < 6; i++) {
    printf("%d", i + 1);
    for (int j = 0; j < 7; j++) {
      printf("|%c", board[i][j]);
    }
    printf("|\n");
  }

  printf(" ---------------\n\n");
}

void error_exit(const char *msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(EXIT_FAILURE);
}

void usage() {
  fprintf(stderr, "Usage: %s address port\n", progname);
  exit(EXIT_FAILURE);
}


void *send_mesg(void *arg)
{
  int server_sockfd = *((int *)arg);

  FILE *server_sockfile = fdopen(server_sockfd, "r+");
//  fputs("Hallo!!!\n", server_sockfile);
//    fflush(server_sockfile);
  char buffer[100];
  char *message; // = fgets(buffer, sizeof(buffer), server_sockfile);

//  printf("%s", message);




  while(1)
  {
	fgets(buffer, BUF, stdin);
	fputs(buffer, server_sockfile);
	fflush(server_sockfile);
	printf("\nwrote to server: %s\n", buffer);

	if(strcmp(buffer, "quit\n") == 0){
		break;
	}
/*
	fgets(buffer, BUF, server_sockfile);
	printf("\ngot from server: %s", buffer);
*/


//	sleep(1);
  }

  run = 0;

  fclose(server_sockfile);

}

void *recive_mesg(void* arg)
{
  int server_sockfd = *((int *)arg);

  FILE *server_sockfile = fdopen(server_sockfd, "r+");
//  fputs("Hallo!!!\n", server_sockfile);
//    fflush(server_sockfile);
  char buffer[100];
  char *message; // = fgets(buffer, sizeof(buffer), server_sockfile);

//  printf("%s", message);




  while(1)
  {
/*
	fgets(buffer, BUF, );
	fputs(buffer, server_sockfile);
	fflush(server_sockfile);
	printf("\nwrote to server: %s\n", buffer);

	if(strcmp(buffer, "quit\n") == 0){
		break;
	}
*/
	fgets(buffer, BUF, server_sockfile);
	printf("\ngot from server: %s", buffer);



//	sleep(1);
  }

  run = 0;

  fclose(server_sockfile);


}


int main(int argc, char **argv) {
  int server_sockfd;
  struct sockaddr_in address;
  // int size;

  // int *entry;

  // char gameBoard [6][7];

  if (argc < 3) {
    usage();
  }
  if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    // printf("Socket wurde angelegt\n");
    error_exit("socket failed");
  }

  address.sin_family = AF_INET;
  address.sin_port = atoi(argv[2]);
  inet_aton(argv[1], &address.sin_addr);
  if (connect(server_sockfd,
              (struct sockaddr *)&address,
              sizeof(address)) == 0) {
    printf("Verbindung mit dem Server (%s) hergestellt\n",
           inet_ntoa(address.sin_addr));
  } else {
    error_exit("connect failed");
  }



  pthread_t thread_recive_mesg;
  if(pthread_create(&thread_recive_mesg, NULL, recive_mesg, (void *)&server_sockfd)< 0)
  {
	printf("problem send recive thread");
	error_exit("msg thread failed");
  }



  pthread_t thread_send_mesg;
  if(pthread_create(&thread_send_mesg, NULL, send_mesg, (void *)&server_sockfd)< 0)
  {
	printf("problem send msg thread");
	error_exit("msg thread failed");
  }


  while(run);



//  printf("\nConnection END\n\n");


//  fputs("quit", server_sockfile);
//  fflush(server_sockfile);



  // do {
  //   size = recv(create_socket, gameBoard, sizeof(gameBoard), 0);
  //   if (size > 0)
  //     buffer[size] = '\0';
  //   //      printf ("Nachricht erhalten: %s\n", buffer);

  //   printBoard(gameBoard);

  //   if (strcmp(buffer, "quit\n")) {
  //     printf("select column: ");
  //     fgets(buffer, BUF, stdin);

  //     //	printf("%s", entry);

  //     send(create_socket, buffer, strlen(buffer), 0);
  //   }
  // } while (strcmp(buffer, "quit\n") != 0);
  // close(create_socket);
  return EXIT_SUCCESS;
}

