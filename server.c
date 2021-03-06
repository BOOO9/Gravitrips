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
#define MAX_GAMEROOM 6 //gameroomm 0 = menue
#define ROWS 6
#define COLS 7
#define STATE 3 //State-row tells which player is allowed to play

typedef struct
{
	int sockfd;
	int player_nmbr;        //checkt loggin
	FILE *client_sockfile;
	int room;               //tells which room, 0 = no room
	int player_room;   //tells 1 = player 1; 2 = player 2 or higher than 2 == viewer

}player_t;

player_t players[MAX_USER+1];

char symbols[] = {' ', 'X', 'O', '4'};
/* Symbols:
 *           ' ' = free Space
 *           'X' = token Player 1
 *           'O' = token Player 2
 *           '4' = Four in a row
 */

typedef struct
{
  int gameboard[ROWS][COLS+1];
}gameroom_t;

gameroom_t gameroom[MAX_GAMEROOM];

int max_rounds; //defines rounds to play till win
int users_in_room[MAX_GAMEROOM];
char *progname;
int user_count;


/* gameroom[x].gameboard[ROWS][COLS]
 *
 *           0 = free Space = ' '
 *           1 = token Player 1 = 'X'
 *           2 = token Player 2 = 'O'
 *           3 = Four in a row = '4'
 *
 * INFO COL = Column 7, easy way to send data to client
 * 	 row 0 = cur_round
 *   row 1 = victorys player X
 *   row 2 = victorys player O
 *   row 3 = permission = which player can play (1= Player 1/2 = Player 2) start value = 1;
 *   row 4 = max rounds
 *
 */


void clear_gameboard(int nmbr, int rows, int cols);  //nmbr = which board to clear
void setToken(char col[], int room_nmbr, int player);
void error_exit(const char *msg);
void usage();
void *handle_client(void *arg);
int start_server(int port);
void mark_four(int room_nbr, int rs, int cs, int dr, int dc);
int search_4_four(int room_nmbr, int player);
void send_board_to_user(int cur_room);


/*main*/

int main(int argc, char **argv)
{

  for(int i = 0; i <= MAX_USER+1; i++)
  {
    players[i].player_nmbr = -1;
    players[i].room = 0;
  }


  for(int i = 0; i < MAX_GAMEROOM; i++)
  {
    clear_gameboard(i, ROWS, COLS);
    gameroom[i].gameboard[STATE][COLS] = 1;
    users_in_room[i] = 0;
  }
  
  user_count = 0;
  progname = argv[0];
  if (argc < 3) usage();

  int port = atoi(argv[1]);
  max_rounds = atoi(argv[2]);
  if (port == 0) usage();

  start_server(port);
 
  return 0;
}


/*functions*/

void setToken(char col[], int room_nmbr, int player)
{
  int row = 0;

  int set = atoi(col);
  
  set--;

  printf("Player %d is putting on col %d in Room: --%d--\n", players[room_nmbr].player_nmbr, set+1, room_nmbr);

	// switches the state of the player who is allowed to play
	if(gameroom[room_nmbr].gameboard[STATE][COLS] == 1) gameroom[room_nmbr].gameboard[STATE][COLS] = 2;
	else if(gameroom[room_nmbr].gameboard[STATE][COLS] == 2) gameroom[room_nmbr].gameboard[STATE][COLS] = 1;


  for(row = 0; row < ROWS; row++)
  {
    if(gameroom[room_nmbr].gameboard[row][set] > 0) break;
  }

	gameroom[room_nmbr].gameboard[row - 1][set] = player;
}

void error_exit(const char *msg)
{
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(EXIT_FAILURE);
}

void usage()
{
  fprintf(stderr, "Usage: %s port number of games\n", progname);
  exit(EXIT_FAILURE);
}

void *handle_client(void *arg)
{
  int sockfd = *((int *)arg);

  int cur;
  int cur_room = 0;
  int winner = 0;


  int player_gone;

  //assigns to current user a sockfd
  for(int i = 1; i < MAX_USER+1; i++)
  {
    if(players[i].player_nmbr < 0)
    {
      players[i].sockfd = sockfd;
      players[i].player_nmbr = i;
      players[i].room = 0;
      cur = i;

      break;
    }
  }

  FILE *client_sockfile = fdopen(players[cur].sockfd , "r+");

  players[cur].client_sockfile = fdopen(players[cur].sockfd , "r+");

  char buffer[100];
  char *message;


  while(1)
  {
    if(players[cur].room == 0) //player is in no room, send him room options
    {
      fwrite(users_in_room, sizeof(int), sizeof(users_in_room), players[cur].client_sockfile);
      fflush(players[cur].client_sockfile);

      message = fgets(buffer, sizeof(buffer), client_sockfile);

      if (message == NULL)
      {
        printf("userinputt NULL\n");
        printf("disconnect user %d\n", players[cur].player_nmbr);
        goto player_left;
      }
      
      cur_room = atoi(message);
      players[cur].room = cur_room;
      users_in_room[cur_room]++;
      players[cur].player_room = users_in_room[cur_room];
      gameroom[cur_room].gameboard[4][COLS] = max_rounds;


      //gives the client his player number
      fprintf(players[cur].client_sockfile, "%d", users_in_room[cur_room]);


    }

    else //player is in room, send board
    {

      while(gameroom[cur_room].gameboard[0][COLS] < max_rounds) //play maxrounds, until game is over
      {

        send_board_to_user(cur_room);

        message = fgets(buffer, sizeof(buffer), client_sockfile);

        if (message == NULL)
        {
          printf("userinputt NULL\n");
          printf("disconnect user %d\n", players[cur].player_nmbr);

          if(players[cur].player_room > 2) goto spectator_left;

          gameroom[cur_room].gameboard[0][COLS] = -1;
          player_gone = 1;
          goto player_left;
        }

        if(atoi(message) == -1)
        {
          gameroom[cur_room].gameboard[0][COLS] = -2;
          player_gone = 1;
          goto player_left;
        }

        //sets the token of the player in the appropriate gameboard field
        setToken(message, cur_room, players[cur].player_room);

        //searches for 4 tokens in a row, changes the four row to the number '4' and returns the number of the winning player
        winner = search_4_four(cur_room, players[cur].player_room);


        if(winner > 0)
        {
          gameroom[cur_room].gameboard[winner][COLS]++;

          send_board_to_user(cur_room);

          gameroom[cur_room].gameboard[0][COLS]++;

          printf("\n\n!!! We have a winner for the round: Player %d (%c)\n !!!", winner, symbols[winner]);

          clear_gameboard(cur_room, ROWS, COLS);

          message = fgets(buffer, sizeof(buffer), client_sockfile); //wait till client leave room

          if (message == NULL)
          {
            printf("userinputt NULL\n");
            printf("disconnect user %d\n", players[cur].player_nmbr);
            goto player_left;
          }
          winner = 0;
        }

      }//while round loop


      if(players[cur].room > 0)
      {
        clear_gameboard(cur_room, ROWS, COLS+1);
        gameroom[cur_room].gameboard[STATE][COLS] = 1;
        players[cur].room = 0; //player is in no room, send him room options
        gameroom[cur_room].gameboard[0][COLS] = 0;
        users_in_room[cur_room] = 0;
      }

      goto player_left;

    }


 }

  player_left:

  players[cur].player_nmbr = -1;
  players[cur].room = 0; //player is in no room, send him room options

  if(player_gone == 1) send_board_to_user(cur_room);

  player_gone = 0;

  clear_gameboard(cur_room, ROWS, COLS+1);

  gameroom[cur_room].gameboard[STATE][COLS] = 1;


  spectator_left:

  players[cur].room = 0; //player is in no room, send him room options
  users_in_room[cur_room]--;

  printf("Someone left the server\n");
  user_count--;

  players[cur].player_nmbr = -1;

  fclose(players[cur].client_sockfile);

  return 0;
}

int start_server(int port)
{
  int server_sockfd, client_sockfd;
  socklen_t addrlen;
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

  pthread_t thread_id;

  while(1)
  {
      client_sockfd = accept(server_sockfd, (struct sockaddr *)&address, &addrlen);

      if (client_sockfd > 0)
      {
        user_count++;

        if(user_count < MAX_USER+1) printf("Connection (%s) established\n", inet_ntoa(address.sin_addr));

        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_sockfd) < 0)   
            error_exit("pthread_create failed");

      }
      else error_exit("accept client");

      if(user_count > MAX_USER)
      {
        printf("\n\nSERVER IS FULL\n\n");
        close(client_sockfd);
        user_count--;
      }

      sleep(1);
  }
  close(server_sockfd);


  return 0;
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

//the basic idea of this function was taken from: https://trainyourprogrammer.de/c-A30-L1-4-gewinnt-fuer-die-konsole.html
void mark_four(int room_nbr, int rs, int cs, int dr, int dc)
{
    int i;

    for (i = 0; i < 4; i++) 
        gameroom[room_nbr].gameboard[rs+i*dr][cs+i*dc] = 3;
}

//the basic idea of this function was taken from: https://trainyourprogrammer.de/c-A30-L1-4-gewinnt-fuer-die-konsole.html
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

void clear_gameboard(int nmbr, int rows, int cols)
{
    for (int j = 0; j < rows; j++)
    {
      for (int k = 0; k < cols; k++)
      {
        gameroom[nmbr].gameboard[j][k] = 0;
      }
    }
}


