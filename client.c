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

#define MAX_GAMEROOM 6
#define BUF 1024
#define ROWS 6
#define COLS 7
#define INDENT "    "

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

int board[ROWS][COLS+1];
int users_in_room[MAX_GAMEROOM];
int state = 0;          //defines what to do (0 = menu, 1 = in game as player, 2 = in game as viewer)
int permission = 1;         //defines whose turn it is to play; 1 = player 1, 2 = player 2
int who_am_i = 0;
char *progname;
char *ip_adress;

char symbols[] = {' ', 'X', 'O', '4'};


void printBoard(int board[ROWS][COLS+1]);
void error_exit(const char *msg);
void usage();
void *send_mesg(void *arg);
void *recive_mesg(void* arg);
void menu(FILE* server_sockfile);   // TODO funciton when Player in menu modz
void game();    // TODO function when Player in game mode
int start_server(int port);
int check_userinput(int low, int high, char* user_input); //checks if correct input, only for int, needs range, returns input if correct or -1


/* main */
int main(int argc, char **argv) //TODO start_server funciton um die main kürzer zu machen
{
  int port;

  if(argc < 2 || argc > 3)
  {
    usage();
  }
  if(argc == 2)
  {
    port = atoi(argv[1]);
    ip_adress = "127.0.0.1";
  }else{
    ip_adress = argv[1];
    port = atoi(argv[2]);
  }


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
  fprintf(stderr, "Usage: %s [ip address] [port]\n", progname);
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
  address.sin_addr.s_addr = inet_addr(ip_adress);

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
  int input;

  sleep(1);

  while(1)
  {
    switch(state)
    {
      case 0:      //in menu

        fgets(buffer, BUF, stdin);

        input = check_userinput(1, MAX_GAMEROOM-1, buffer);

        if(input > 0)
        {
          fputs(buffer, server_sockfile);
          fflush(server_sockfile);
          if(who_am_i >= 2)
          {
            state = 2;
          }
          else
          {
            state = 1;
          }
        }

        //TODO better solution for sleep(1)
        sleep(1);

        break;

      case 1:       //in room as player
        fgets(buffer, BUF, stdin);

        if(board[1][COLS] + board[2][COLS] == board[4][COLS] || board[0][COLS] == -1) break;

        permission = board[3][COLS];  //tells who has the permission to play

        if(permission != who_am_i)
        {
          printf("It's not your turn, please wait for your opponnent to play!\n");
          break;
        }

        input = check_userinput(1, COLS, buffer);

        if(input > 0 && (board[1][COLS]+board[2][COLS]) < board[4][COLS])
        {
          fputs(buffer, server_sockfile);
          fflush(server_sockfile);
        }
        break;

      case 2:     //in room as a viewer
        sleep(3); //TODO suabere lösung
        break;
    }

    if(strcmp(buffer, "quit\n") == 0) break;

//    if(board[1][COLS] + board[2][COLS] == board[4][COLS]) break;

  }

  //  fclose(server_sockfile);

  return 0;
}

void *recive_mesg(void* arg)
{
  int server_sockfd = *((int *)arg);

  FILE *server_sockfile = fdopen(server_sockfd, "r+");

  while(1)
  {

    switch(state) {
       case 0: //user is in menu

        fread(users_in_room, sizeof(int), sizeof(users_in_room), server_sockfile);
        menu(server_sockfile);
        fscanf(server_sockfile, "%d", &who_am_i); // tells if client is player 1/2 or viewer > 2
        fread(board, sizeof(int), sizeof(board), server_sockfile);
        printBoard(board);

        if(who_am_i < 3)
        {
        printf("I am Nr.: --%d--, and Player --%d-- (Permission) is allowed to play\n\n", who_am_i, permission); 
        }else{printf("you are a spectator\n\n");}
      break;

      case 1:     //in game as player

        fread(board, sizeof(int), sizeof(board), server_sockfile);
        printBoard(board);
        permission = board[3][COLS];  //tells who has the permission to play

        if(board[1][COLS]+board[2][COLS] == board[4][COLS])
        {
          printf("\n***GAME OVER***\n");
          printf("Press Enter to leave\n\n");
          goto end;
        }
        if(board[0][COLS] == -1)
        {
          printf("\n***GAME OVER***\n");
          printf("\nYour opponend is gay and left, you win\n\n");
          goto end;
        }

        printf("I am Nr.: --%d--, and Player --%d-- (Permission) is allowed to play\n\n", who_am_i, permission); 

      break;

      case 2:     //in game as viewer

        fread(board, sizeof(int), sizeof(board), server_sockfile);
        printBoard(board);
        printf("you are a spectator\n\n");
        if(board[1][COLS]+board[2][COLS] == board[4][COLS] || board[0][COLS] == -1)
        {
          printf("\n***GAME OVER***\n");
          goto end;
        }
        if(board[1][COLS]+board[2][COLS] == board[4][COLS] || board[0][COLS] == -1)
        {
          printf("\n***GAME OVER***\n");
          printf("\nA player left\n\n");
          goto end;
        }

        break;
    }
  }
  end:

  fclose(server_sockfile);

  return 0;
}




int check_userinput(int low, int high, char* user_input)
{
  int input = atoi(user_input);

  if(input >= low && input <= high)
  {
    return input;
  }
  else
  {
    printf("choose number between %d and %d \n", low, high);
    return -1;
  }
}



void menu(FILE* server_sockfile)
{

  //  system("cls");
  printf("\e[1;1H\e[2J");

  printf("---- Menu ---- \n");

  for(int i = 1; i < MAX_GAMEROOM; i++)
  {
    printf("\n users in room %d: %d", i, users_in_room[i]);
  }

  printf("\nChoose a room (1 - 5): ");
}


void printBoard(int board[ROWS][COLS+1])
{
  int i = 0;
  int j = 0;

  printf("\e[1;1H\e[2J");


  printf(" \t  ");

  for(i = 1; i <= COLS; i++)
    printf("|%d", i);

  printf("|\n");


  for(i = 0; i < ROWS; i++)
  {
    printf("\t%d ", i + 1);

    for (j = 0; j < COLS; j++) printf("|%c", symbols[board[i][j]]);

    printf("|  INFO %d\n", board[i][COLS]);

  }

  printf("\t  ---------------\n\n");

  printf("Round %d of %d, Player X victories: %d, Player O victories: %d\n\n",board[0][COLS]+1, board[4][COLS],board[1][COLS], board[2][COLS]);
}










