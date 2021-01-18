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
#include <semaphore.h>
#include <sys/time.h>



/*declaration*/

#define MAX_GAMEROOM 6
#define ROWS 6
#define BUF 1024
#define COLS 7
#define max_time2think 10

int board[ROWS][COLS+1];
int users_in_room[MAX_GAMEROOM];
int state = 0;          //defines what to do (0 = menu, 1 = in game as player, 2 = in game as viewer)
int permission = 1;         //defines whose turn it is to play; 1 = player 1, 2 = player 2
int who_am_i = 0;
char *progname;
char *ip_adress;

sem_t mutex;

char symbols[] = {' ', 'X', 'O', '4'};


void printBoard(int board[ROWS][COLS+1]);
void error_exit(const char *msg);
void usage();
void *send_mesg(void *arg);
void *recive_mesg(void* arg);
void menu(FILE* server_sockfile);   //funciton when Player is in menue
void game();    //function when Player in game mode
int start_server(int port);  //start_server funciton um die main kürzer zu machen
int check_userinput(int low, int high, char* user_input); //checks if correct input, only for int, needs range, returns input if correct or -1


/* main */
int main(int argc, char **argv)
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
  }

  else
  {
    ip_adress = argv[1];
    port = atoi(argv[2]);
  }


  int server_sockfd = start_server(port);

  sem_init(&mutex, 0, 1);

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

  char buffer[20];
  int input;

 //for timeout:

  fd_set          input_set;
  struct timeval  timeout;
  int             ready_for_reading = 0;

  FD_ZERO(&input_set );

  FD_SET(0, &input_set);


  timeout.tv_sec = max_time2think;    // max_time2think seconds
  timeout.tv_usec = 0;    // 0 milliseconds

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

        sleep(1);

        break;

      case 1:       //in room as player

        sem_wait(&mutex);

        if(board[1][COLS] + board[2][COLS] == board[4][COLS] || board[0][COLS] == -1) goto ends;

        do
        {

          // select() = 0 when time is over, 1 = when input succesfully taken 
          ready_for_reading = select(1, &input_set, NULL, NULL, &timeout);

          if (ready_for_reading == -1) 
          {
            printf("Unable to read your input\n");
            goto ends;
          } 

          if (ready_for_reading == 1) 
          {
            read(0, buffer, 19);
          } 

          else if(ready_for_reading == 0)
          {

            if(permission == who_am_i) printf("\nYou needed too much time! :(\nYou lost!\n***GAME OVER***\n");

            strcpy(buffer, "-1");      

            fputs(buffer, server_sockfile);
            fflush(server_sockfile);

            exit(-1); 
     
          }
          
          input = check_userinput(1, COLS, buffer);
        
        }while (input <= 0);

        if(input > 0 && (board[1][COLS]+board[2][COLS]) < board[4][COLS])
        {
          fputs(buffer, server_sockfile);
          fflush(server_sockfile);
        }

        timeout.tv_sec = max_time2think;

        break;

      case 2:     //in room as a viewer
        
        fgets(buffer, BUF, stdin);
        if(board[1][COLS] + board[2][COLS] == board[4][COLS] || board[0][COLS] == -1) goto ends;
        
        break;
    }

  }

  ends:
  fclose(server_sockfile);

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

          if(who_am_i == permission) printf("Please type in the column you want to play: \n");
          else if(who_am_i != permission) printf("Please wait for the your opponnent to play!\n");

          state = 1;

        }
        else
        {
          printf("You are a spectator\n\n");
          state = 2;
        }

      break;

      case 1:     //in game as player

        fread(board, sizeof(int), sizeof(board), server_sockfile);
        printBoard(board);
        permission = board[3][COLS];  //tells who has the permission to play

        //abort condition ronuds are over
        if(board[1][COLS]+board[2][COLS] == board[4][COLS])
        {
          printf("\n***GAME OVER***\n");
          goto end;
        }

        //abort condition Player left with Ctrl-C
        if(board[0][COLS] == -1)
        {
          printf("\n***GAME OVER***\n");
          printf("\nYour opponnent left, you win! \n\n");
          goto end;
        }

        //abort condition timeout from other player
        if(board[0][COLS] == -2)
        {
          printf("Your opponnent needed too much time!\nYou won!\n***GAME OVER***\n");
          goto end;
        }

        if(who_am_i == permission) 
        {
          printf("Please type in the column you want to play: \n");
          sem_post(&mutex);
        }  

        else if(who_am_i != permission) printf("Please wait for the your opponnent to play!\n");

        break;

      case 2:     //in game as viewer

        fread(board, sizeof(int), sizeof(board), server_sockfile);
        printBoard(board);
        printf("you are a spectator\n\n");

        if(board[1][COLS]+board[2][COLS] == board[4][COLS])
        {
          printf("\n***GAME OVER***\n");
          goto end;
        }

        if(board[0][COLS] == -1)
        {
          printf("\n***GAME OVER***\n");
          printf("\nA player left!\n\n");
          goto end;
        }

        if(board[0][COLS] == -2)
        {
          printf("\n***GAME OVER***\n");
          printf("\nA player timed out!\n\n");
          goto end;
        }

        break;
    }
  }
  end:

  fclose(server_sockfile);
  sem_post(&mutex);
  exit(-1);

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

  system("clear");

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

  system("clear");


  printf(" \t  ");

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

  printf("Round %d of %d, Player X victories: %d, Player O victories: %d\n\n",board[0][COLS]+1, board[4][COLS],board[1][COLS], board[2][COLS]);
}










