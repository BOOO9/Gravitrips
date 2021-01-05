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


/*declaration*/

#define BUF 1024

int run = 1;
char *progname;

void printBoard(char board[6][7]);
void error_exit(const char *msg);
void usage();
void *send_mesg(void *arg);
void *recive_mesg(void* arg);

/* main */
int main(int argc, char **argv)
{
  int server_sockfd;
  struct sockaddr_in address;

  if (argc < 3)
  {
    usage();
  }

  if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    error_exit("socket failed");
  }

  address.sin_family = AF_INET;
  address.sin_port = atoi(argv[2]);
  inet_aton(argv[1], &address.sin_addr);

  if (connect(server_sockfd,
              (struct sockaddr *)&address,
              sizeof(address)) == 0)
  {
    printf("Verbindung mit dem Server (%s) hergestellt\n", inet_ntoa(address.sin_addr));
  }

  else
  {
    error_exit("connect failed");
  }

  pthread_t thread_recive_mesg;

  if(pthread_create(&thread_recive_mesg,
                    NULL,
                    recive_mesg,
                    (void *)&server_sockfd)< 0)
  {
	  printf("problem send recive thread");
	  error_exit("msg thread failed");
  }


  pthread_t thread_send_mesg;
  if(pthread_create(&thread_send_mesg,
                    NULL,
                    send_mesg,
                    (void *)&server_sockfd) < 0)
  {
	  printf("problem send msg thread");
	  error_exit("msg thread failed");
  }

  while(run);

  return EXIT_SUCCESS;
}


/*functions*/

void printBoard(char board[6][7])
{
//  printf("\e[1;1H\e[2J");

  printf("  1 2 3 4 5 6 7\n");
  printf(" ---------------\n");

  for (int i = 0; i < 6; i++)
  {
    printf("%d", i + 1);

    for (int j = 0; j < 7; j++) printf("|%c", board[i][j]);

    printf("|\n");
  }

  printf(" ---------------\n\n");
}

void error_exit(const char *msg)
{
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(EXIT_FAILURE);
}

void usage()
{
  fprintf(stderr, "Usage: %s address port\n", progname);
  exit(EXIT_FAILURE);
}

void *send_mesg(void *arg)
{
  int server_sockfd = *((int *)arg);

  FILE *server_sockfile = fdopen(server_sockfd, "r+");

  char buffer[100];
  char *message; // = fgets(buffer, sizeof(buffer), server_sockfile);

  while(1)
  {
    fgets(buffer, BUF, stdin);
    fputs(buffer, server_sockfile);
    fflush(server_sockfile);
    printf("\nwrote to server: %s\n", buffer);

    if(strcmp(buffer, "quit\n") == 0) break;

  }

  run = 0;
  fclose(server_sockfile);
}

void *recive_mesg(void* arg)
{
  int server_sockfd = *((int *)arg);

  FILE *server_sockfile = fdopen(server_sockfd, "r+");

  char buffer[100];
  char *message; // = fgets(buffer, sizeof(buffer), server_sockfile);

  char board[6][7];


  while(1)
  {

	  fread(board, sizeof(char), sizeof(board),server_sockfile);
	  printBoard(board);

//	  fgets(buffer, BUF, server_sockfile);
//	  printf("\ngot from server: %s", buffer);
  }

  run = 0;

  fclose(server_sockfile);
}


