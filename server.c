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
#define COLS 7


typedef struct
{
	int sockfd;
	int player_nmbr;        //checkt loggin
	FILE *client_sockfile;
	int room;               //tells which room, 0 = no room
	int player_room;   //tells if player 1 or player 2 or higher than 2 == viewer

}player_t;

player_t players[MAX_USER];

char symbols[] = {' ', 'X', 'O', '4'};
/* Symbols:
 *           ' ' = free Space
 *           'X' = token Player 1 
 *           'O' = token Player 2 
 *           '4' = Four in a row
 */

typedef struct
{
  int gameboard[ROWS][COLS];
//  int users_in_room;      //tells the numbers of users in a room

}gameroom_t;

gameroom_t gameroom[MAX_GAMEROOM];

int users_in_room[MAX_GAMEROOM];

/* gameroom[x].gameboard[ROWS][COLS]
 *
 *           0 = free Space = ' '
 *           1 = token Player 1 = 'X'
 *           2 = token Player 2 = 'O'
 *           3 = Four in a row = '4'

*/


//TODO
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;


char *progname;
int user_count;

//XXX
void clear_gameboard(int nmbr);  //nmbr = which board to clear
void setToken(char col[], int room_nmbr, int player);
void error_exit(const char *msg);
void usage();
void *handle_client(void *arg);
int start_server(int port);
int mark_four(int room_nbr, int rs, int cs, int dr, int dc);
int search_4_four(int room_nmbr, int player);
void send_board_to_user(int cur_room);


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
    users_in_room[i] = 0;
  }
  
  user_count = 0;
  progname = argv[0];
  if (argc < 2) usage();

  int port = atoi(argv[1]);

  if (port == 0) usage();

  start_server(port);
 
  return 0;
}


/*functions*/

void setToken(char col[], int room_nmbr, int player)
{
  int row = 0;

  printf("FIELD string: %s\n", col);


  int set = atoi(col);
  if(set <= 0) 
  {
    printf("\nUser is to stupid to play\n");
    return;
  }
    set--;

  printf("FIELD int: %d // player_nbr: %d\n", set, player);


  for(row = 0; row < ROWS; row++)
  {
    if(gameroom[room_nmbr].gameboard[row][set] > 0) break;
  }

  gameroom[room_nmbr].gameboard[row - 1][set] = player;


 //DEBUG printf gemeboard:

  for(int i = 0; i < MAX_GAMEROOM; i++)
  {
    for(int j = 0; j < ROWS; j++)
    {
      for(int k = 0; k < COLS; k++)
      {
        printf("|%d", gameroom[i].gameboard[j][k]);
      }
      printf("\n");
    }
    printf("room %d \n\n", i);
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
  int cur_room = 0;
  int winner = 0;

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

      for(int i = 0; i<MAX_GAMEROOM; i++) printf("users in room: %d  ", users_in_room[i]);

      fwrite(users_in_room, sizeof(int), MAX_GAMEROOM, players[cur].client_sockfile);
      fflush(players[cur].client_sockfile);


      message = fgets(buffer, sizeof(buffer), client_sockfile);


      if (message == NULL)
      {
        printf("userinputt NULL\n");
	printf("disconnect user %d\n", players[cur].player_nmbr);
	break;
      }
      if(strcmp(buffer, "quit\n") == 0) break;

      cur_room = atoi(message)-1;
      players[cur].room = cur_room;
      users_in_room[cur_room]++;
      players[cur].player_room = users_in_room[cur_room];
    }


    else //player is in room, send board
    {

      //TODO
      //get_userinput(buffer, message, client_sockfile);

      send_board_to_user(cur_room);


      while(winner == 0){
        message = fgets(buffer, sizeof(buffer), client_sockfile);

        if (message == NULL)
        {
          printf("userinputt NULL\n");
          printf("disconnect user %d\n", players[cur].player_nmbr);
          goto client_left;
        }


        if(strcmp(buffer, "quit\n") == 0) break;

        setToken(message, cur_room, players[cur].player_room);

        winner = search_4_four(cur_room, players[cur].player_room);

        send_board_to_user(cur_room);


        if(winner > 0)
        {
          printf("\n\n!!! We have a winner: Player %d (%c)\n !!!", winner, symbols[winner]);
          clear_gameboard(cur_room);
          players[cur].room = 0;
          message = fgets(buffer, sizeof(buffer), client_sockfile); //wait till client leave room

          users_in_room[cur_room] = 0;

          if (message == NULL)
          {
            printf("userinputt NULL\n");
            printf("disconnect user %d\n", players[cur].player_nmbr);
            goto client_left;
          }

        }


      }//while winner == 0 end

    }//else end


    printf("\ngot from client usernmbr %d, fd %d: %s\n", cur, players[cur].player_nmbr, buffer);
  }

  client_left:

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

  setsockopt(server_sockfd, SOL_SOCKET, SOL_SOCKET, &optval, sizeof(int));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = port;

  if (bind(server_sockfd, (struct sockaddr *)&address, sizeof(address)) != 0) 
      error_exit("bind failed");

  listen(server_sockfd, 5);
  addrlen = sizeof(struct sockaddr_in);

  printf("Server is running and waiting...\n");

  // TODO
  pthread_t thread_id;

  while (user_count < MAX_USER)
  {

    client_sockfd = accept(server_sockfd, (struct sockaddr *)&address, &addrlen);

    if (client_sockfd > 0)
    {
      printf("Connection (%s) established\n", inet_ntoa(address.sin_addr));

      user_count++;

      if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_sockfd) < 0)   
          error_exit("pthread_create failed");

    }
    else error_exit("accept client");      // TODO

  sleep(1);
  }
  close(server_sockfd);
}




void send_board_to_user(int cur_room)
{
      for(int i = 1; i <= MAX_USER; i++)
      {
        if(players[i].player_nmbr > 0 && players[i].room == cur_room)
        {

          fwrite(gameroom[cur_room].gameboard, sizeof(int), sizeof(gameroom[cur_room].gameboard), players[i].client_sockfile);
          fflush(players[i].client_sockfile);
        }
      }
}



int mark_four(int room_nbr, int rs, int cs, int dr, int dc)
{
    int i;

    for (i = 0; i < 4; i++) 
        gameroom[room_nbr].gameboard[rs+i*dr][cs+i*dc] += 2;
}

/*
int search_4_four(int room_nbr, int player)
{
    int i, j;

		//int Grid[6][7] = gameroom[room_nbr].gameboard;
     
    // Suche waagrecht    
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS - 3; j++)
        {
            if (Grid[i][j] == player && Grid[i][j+1] == player && Grid[i][j+2] == player && Grid[i][j+3] == player ) 
            {
                mark_four(room_nbr, i, j, 0, +1);
                return player; 
            }
        }
    }

    // Suche senkrecht    
    for (j=0; j<COLS; j++)
    {    
        for (i=0; i<ROWS-3; i++)
        {
            if (Grid[i][j] == player && Grid[i+1][j] == player && Grid[i+2][j] == player && Grid[i+3][j] == player ) 
            {
                mark_four(room_nbr, i, j, +1, 0);
                return player; 
            }
        }
    }

    // Suche diagonal '\'   
    for(i=0; i<ROWS-3; i++)
    {
        for(j=0; j<COLS-3; j++)
        {
            if(Grid[i][j] == player && Grid[i+1][j+1] == player && Grid[i+2][j+2] == player && Grid[i+3][j+3] == player )
            {
                mark_four(room_nbr, i, j, +1, +1);
                return player; 
            }
        }
    }

    // Suche diagonal '/'
    for (i=0; i<ROWS-3; i++)
    {
        for (j=COLS-4; j<COLS; j++)
        {
            if (Grid[i][j] == player && Grid[i+1][j-1] == player && Grid[i+2][j-2] == player && Grid[i+3][j-3] == player)
            {
                mark_four(room_nbr, i, j, +1, -1);
                return player;
            }
        }
    }

    return 0;
}
*/

int search_4_four(int room_nbr, int player)
{
    int i, j;

     
    // Suche waagrecht    
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS - 3; j++)
        {
            if (gameroom[room_nbr].gameboard[i][j] == player && gameroom[room_nbr].gameboard[i][j+1] == player && gameroom[room_nbr].gameboard[i][j+2] == player && gameroom[room_nbr].gameboard[i][j+3] == player ) 
            {
                mark_four(room_nbr, i, j, 0, +1);
                return player; 
            }
        }
    }

    // Suche senkrecht    
    for (j=0; j<COLS; j++)
    {    
        for (i=0; i<ROWS-3; i++)
        {
            if (gameroom[room_nbr].gameboard[i][j] == player && gameroom[room_nbr].gameboard[i+1][j] == player && gameroom[room_nbr].gameboard[i+2][j] == player && gameroom[room_nbr].gameboard[i+3][j] == player ) 
            {
                mark_four(room_nbr, i, j, +1, 0);
                return player; 
            }
        }
    }

    // Suche diagonal '\'   
    for(i=0; i<ROWS-3; i++)
    {
        for(j=0; j<COLS-3; j++)
        {
            if(gameroom[room_nbr].gameboard[i][j] == player && gameroom[room_nbr].gameboard[i+1][j+1] == player && gameroom[room_nbr].gameboard[i+2][j+2] == player && gameroom[room_nbr].gameboard[i+3][j+3] == player )
            {
                mark_four(room_nbr, i, j, +1, +1);
                return player; 
            }
        }
    }

    // Suche diagonal '/'
    for (i=0; i<ROWS-3; i++)
    {
        for (j=COLS-4; j<COLS; j++)
        {
            if (gameroom[room_nbr].gameboard[i][j] == player && gameroom[room_nbr].gameboard[i+1][j-1] == player && gameroom[room_nbr].gameboard[i+2][j-2] == player && gameroom[room_nbr].gameboard[i+3][j-3] == player)
            {
                mark_four(room_nbr, i, j, +1, -1);
                return player;
            }
        }
    }

    return 0;
}




void clear_gameboard(int nmbr)
{
    for (int j = 0; j < ROWS; j++)
    {
      for (int k = 0; k < COLS; k++)
      {
        gameroom[nmbr].gameboard[j][k] = 0;
      }
    }
}


