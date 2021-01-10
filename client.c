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
#define ROWS 6
#define COLS 7
#define INDENT "    "

int state = 0; //defines what to do (0 = menue, 1 = in game as player, 2 = in game as viewer)
int run = 1;
char *progname;

char symbols[] = {' ', 'X', 'O', '4'};


void printBoard(int board[6][7]);
void error_exit(const char *msg);
void usage();
void *send_mesg(void *arg);
void *recive_mesg(void* arg);
void menue();   // TODO funciton when Player in menu modz
void game();    // TODO function when Player in game mode
void get_user_input_to_server(char* buffer, FILE* server_sockfile);// TODO
int start_server(int port);


/* main */
int main(int argc, char **argv) //TODO start_server funciton um die main kürzer zu machen
{

  if (argc < 2)
  {
    usage();
  }

  int port = atoi(argv[1]); 

  int server_sockfd = start_server(port);

  pthread_t thread_recive_mesg;

  if(pthread_create(&thread_recive_mesg, NULL, recive_mesg, (void *)&server_sockfd) < 0)
  {
	  printf("problem send recive thread");
	  error_exit("msg thread failed");
  }

  pthread_t thread_send_mesg;
  if(pthread_create(&thread_send_mesg, NULL, send_mesg, (void *)&server_sockfd) < 0)
  {
	  printf("problem send msg thread");
	  error_exit("msg thread failed");
  }

  
  pthread_join(thread_send_mesg, NULL);
  pthread_join(thread_recive_mesg, NULL);

     
  
  //while(run);// sleep(2);

  return EXIT_SUCCESS;
}


/*functions*/


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

int start_server(int port)
{

  int server_sockfd;
  struct sockaddr_in address;

  if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    error_exit("socket failed");
  }

  address.sin_family = AF_INET;
  address.sin_port = port;
  //to choose the ipadress: inet_aton(argv[1], &address.sin_addr);
  address.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(server_sockfd, (struct sockaddr *)&address, sizeof(address)) == 0)
  {
    printf("Verbindung mit dem Server (%s) hergestellt\n", inet_ntoa(address.sin_addr));
  }
  else
  {
    error_exit("connect failed");
  }

  return server_sockfd;
}

void *send_mesg(void *arg)
{
  int server_sockfd = *((int *)arg);

  FILE *server_sockfile = fdopen(server_sockfd, "r+");

  char buffer[100];
  char *message; // = fgets(buffer, sizeof(buffer), server_sockfile);

  sleep(1);

  while(1)
  {

    //sleep(1);
    switch(state)
    {
      case 0:  
        get_user_input_to_server(buffer, server_sockfile);
        if(atoi(buffer) > 0) state = 1; //TODO idea: check if room is full, to get right state
        break;
      case 1:
        get_user_input_to_server(buffer, server_sockfile);
        break;
      case 2:
        scanf("%d", &state);
        break;
    }




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

  int board[6][7];
  
  //fread(board, sizeof(char), sizeof(board), server_sockfile);
  
  while(1)
  {

    //sleep(1);
    switch(state)
    {
      case 0:
        menue();
        fread(buffer, sizeof(int), sizeof(board), server_sockfile);
        break;
      case 1:
        fread(board, sizeof(int), sizeof(board), server_sockfile);
        printf("you are a player\n");
        sleep(2);
        printBoard(board);
        break;
      case 2:
        fread(board, sizeof(int), sizeof(board), server_sockfile);
        printf("you are a viewer, press 0 to leave\n\n");
        printBoard(board);
        break;
    }
  
  //	  fgets(buffer, BUF, server_sockfile);
  //	  printf("\ngot from server: %s", buffer);
  }

  run = 0;

  fclose(server_sockfile);
}



void menue()
{
  printf("\e[1;1H\e[2J");
  printf("---- Menu ---- \n Choose a room (1 - 5): ");
}


void printBoard(int board[6][7])
{
  int i = 0;
  int j = 0;

  printf("\e[1;1H\e[2J");

  printf("\t  ");

  for(i = 1; i <= COLS; i++)
    printf("|%d", i);

  printf("|\n");


  for(i = 0; i < ROWS; i++)
  {
    printf("\t%d ", i + 1);

    for (j = 0; j < COLS; j++) printf("|%c", symbols[board[i][j]]);

    printf("|\n");
  }

  printf("\t  ---------------\n\n");
}




void get_user_input_to_server(char* buffer, FILE* server_sockfile)
{
  fgets(buffer, BUF, stdin);
  fputs(buffer, server_sockfile);
  fflush(server_sockfile);
  printf("\nwrote to server: %s\n", buffer);
}









