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
#define MAX_GAMEROOM 8
#define ROWS 6
#define COLUMNS 7

#define a 3

typedef struct
{
	int sockfd;
	int player_nmbr;        //checkt loggin
	FILE *client_sockfile;
	int room;               //tells which room, 0 = no room
	int player_nmbr_room;   //tells which (X or O player or viewer

}player_t;

player_t players[MAX_USER];

char symbols[] = {' ', 'X', 'O', '4'};
/* Symbols:
 *           0 = ' ' = free Space
 *           1 = 'X' = Player 1 token
 *           2 = 'O' = Player 2 token
 *           3 = '4' = Four in a row
 */

typedef struct
{
	char gameboard[ROWS][COLUMNS];
  //int gameboard[ROWS][COLUMNS];

}gameroom_t;

gameroom_t gameroom[MAX_GAMEROOM];

/* gameroom[x].gameboard[ROWS][COLS]
 *
 *           0 = ' ' = free Space
 *           1 = 'X' = Player 1 token
 *           2 = 'O' = Player 2 token
 *           3 = '4' = Four in a row
 */



pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;


char *progname;
int user_count;

void clear_gameboard(int nmbr);  //nmbr = which board to clear
void setToken(char field[], int room_nmbr);
void error_exit(const char *msg);
void usage();
void *handle_client(void *arg);
int start_server(int port);
void get_userinput(char buffer[], char* message, FILE* client_sockfile);
// mark_four()
// seacrch_4_four()
//

/*main*/

int main(int argc, char **argv)
{
  for(int i = 0; i <= MAX_USER; i++)
  {
    players[i].player_nmbr = -1;
    players[i].room = 0;
  }


  for(int i = 0; i < MAX_GAMEROOM; i++)
  {
    clear_gameboard(i);
  }
  
  user_count = 0;
  progname = argv[0];
  if (argc < 2) usage();

  int port = atoi(argv[1]);

  if (port == 0) usage();

  return start_server(port);
}


/*functions*/

void setToken(char field[], int room_nmbr)
{
  int row = 0;

  printf("FIELD string: %s\n", field);


  int set = atoi(field);
  set--;

  printf("FIELD int: %d\n", set);


  for(row = 0; row < ROWS; row++)
  {
    if(gameroom[room_nmbr].gameboard[row][set] == 'X') break;
  }

  gameroom[room_nmbr].gameboard[row - 1][set] = 'X';

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
  int cur_room = 0;


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

  char buffer[100];
  char *message;
  
    
  //  players[cur].room = a;
  
  while(1)
  {
    if(players[cur].room == 0) //player is in no room, send him room options
    {
      fwrite(gameroom[cur].gameboard, sizeof(char), sizeof(gameroom[cur].gameboard), players[cur].client_sockfile);
      fflush(players[cur].client_sockfile);

      message = fgets(buffer, sizeof(buffer), client_sockfile);
      cur_room = atoi(message);
      players[cur].room = cur_room;

    }else{ //player is in room, send board
    
  //      get_userinput(buffer, message, client_sockfile);
  
      message = fgets(buffer, sizeof(buffer), client_sockfile);

      if (message == NULL)
      {
        error_exit("userinputt NULL");
      }


      if(strcmp(buffer, "quit\n") == 0) break;

      setToken(message, cur_room);

      for(int i = 1; i <= MAX_USER; i++)
      {
        if(players[i].player_nmbr > 0 && players[i].room == cur_room)
        {
          printf("PRINT TO %d: \n", i);
  //          fputs(message, players[i].client_sockfile);
  //          fflush(players[i].client_sockfile);
  
          fwrite(gameroom[cur_room].gameboard, sizeof(char), sizeof(gameroom[cur_room].gameboard), players[i].client_sockfile);
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


void get_userinput(char buffer[100], char* message, FILE* client_sockfile)
{
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
  
  while (user_count < MAX_USER)
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

  sleep(1);
  }
  close(server_sockfd);
}



void clear_gameboard(int nmbr)
{
    for (int j = 0; j < ROWS; j++)
    {
      for (int k = 0; k < COLUMNS; k++)
      {
        gameroom[nmbr].gameboard[j][k] = ' ';
      }
    }
}


