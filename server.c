/* server.c */
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


/*declarations*/

#define BUF 1024
#define MAX_USER 20
#define MAX_GAMEROOM 5
#define ROWS 6
#define COLUMNS 7

int a = 2;

typedef struct
{
	int sockfd;
	int player_nmbr;  //checkt loggin
	FILE *client_sockfile;
	int room; //sagt welcher gameroom
	int player_nmbr_room; //sagt zuschauer od spieler (und ob x od 0)

} player_t;

player_t players[MAX_USER];



typedef struct
{
	char gameboard[ROWS][COLUMNS];

}gameroom_t;

gameroom_t gameroom[MAX_GAMEROOM];

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

char *progname;
int user_count;

void setToken(char board[ROWS][COLUMNS], char field[]);
void error_exit(const char *msg);
void usage();
void *handle_client(void *arg);
int start_server(int port);


/*main*/

int main(int argc, char **argv)
{
  for(int i = 0; i<= MAX_USER; i++) players[i].player_nmbr = -1;

  for(int i = 0; i<MAX_GAMEROOM; i++)
  {
    for (int j = 0; j < ROWS; j++)
    {
      for (int k = 0; k < COLUMNS; k++)
      {
        gameroom[i].gameboard[j][k] = ' ';
      }
    }
  }

  user_count = 0;
  progname = argv[0];
  if (argc < 2) usage();

  int port = atoi(argv[1]);

  if (port == 0) usage();

  return start_server(port);
}


/*functions*/

void setToken(char board[ROWS][COLUMNS], char field[])
{
  int row = 0;

  printf("FIELD string: %s\n", field);


  int set =  atoi(field);
  set--;

  printf("FIELD int: %d\n", set);


  while (gameroom[a].gameboard[row][set] == ' ') row++;

  gameroom[a].gameboard[row - 1][set] = 'X';


for(int i=0; i<6; i++)
{
  for(int j = 0; j<7; j++) printf("%c",gameroom[a].gameboard[i][j]);
}

}

void error_exit(const char *msg)
{
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(EXIT_FAILURE);
}

void usage()
{
  fprintf(stderr, "Usage: %s port\n", progname);
  exit(EXIT_FAILURE);
}

void *handle_client(void *arg)
{
  int sockfd = *((int *)arg);

  int cur;

  for(int i = 1; i < MAX_USER; i++)
  {
    if(players[i].player_nmbr < 0)
    {
      players[i].sockfd = sockfd;
      players[i].player_nmbr = i;

      cur = i;

      break;
    }
  }

  FILE *client_sockfile = fdopen(players[cur].sockfd , "r+");

  players[cur].client_sockfile = fdopen(players[cur].sockfd , "r+");


  while(1)
  {
    char buffer[100];

    char *message = fgets(buffer, sizeof(buffer), client_sockfile);

    if (message != NULL)
    {
      if(strcmp(buffer, "quit\n") == 0)	break;

	printf("MESSAGE: %s\n",message);

      setToken(gameroom[a].gameboard, message);

      for(int i = 1; i <= MAX_USER; i++)
      {
        if(players[i].player_nmbr > 0)
        {
          printf("PRINT TO %d: \n", i);
//          fputs(message, players[i].client_sockfile);
//          fflush(players[i].client_sockfile);

	  fwrite(gameroom[a].gameboard, sizeof(char), sizeof(gameroom[a].gameboard), players[i].client_sockfile);
          fflush(players[i].client_sockfile);
          printf("got it\n");
        }
      }
    }


    printf("\ngot from client usernmbr %d, fd %d: %s\n", cur, players[cur].player_nmbr, buffer);
  }

  printf("someone left\n");
  user_count--;

  players[cur].player_nmbr = -1;

  fclose(players[cur].client_sockfile);
}

int start_server(int port)
{
  int server_sockfd, client_sockfd;
  socklen_t addrlen;
  ssize_t size;
  struct sockaddr_in address;

  if ((server_sockfd =
       socket(AF_INET, SOCK_STREAM, 0)) < 0)
       error_exit("socket failed");

  const int optval = 1;

  setsockopt(server_sockfd,
             SOL_SOCKET,
             SO_REUSEADDR,
             &optval,
             sizeof(int));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = port;

  if (bind(server_sockfd,
           (struct sockaddr *)&address,
           sizeof(address)) != 0)
           error_exit("bind failed");

  listen(server_sockfd, 5);
  addrlen = sizeof(struct sockaddr_in);

  // TODO
  pthread_t thread_id;
  while (1)
  {

    client_sockfd = accept(server_sockfd,
                          (struct sockaddr *)&address,
                          &addrlen);

    if (client_sockfd > 0)
    {
      printf("Connection (%s) established\n", inet_ntoa(address.sin_addr));

      user_count++;

      if (pthread_create(&thread_id,
                       NULL,
                       handle_client,
                       (void *)&client_sockfd) < 0) 
                       error_exit("pthread_create failed");

    }
    else error_exit("accept client");      // TODO

  }
  close(server_sockfd);
}


