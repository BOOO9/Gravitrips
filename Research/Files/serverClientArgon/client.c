#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 2048
#define MAX_NAMELEN 32



//GLOBAL VARIABLES
volatile sig_atomic_t flag = 0;
int sockFD = 0;
char name[MAX_NAMELEN];


void str_trim_lf(char* arr,int length)
{
    for(int i = 0 ; i < length; i++)
    {
        if(arr[i] == '\n')
        {
            arr[i] == '\0';
            break;
        }
    }

}

void str_overwrite_stdout()
{
    printf("\r%s", "> ");
    fflush(stdout);

}

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void send_message_handler()
{
    char message[BUFFER_SIZE] = {};
	char buffer[BUFFER_SIZE + 32] = {};

  while(1) {
  	str_overwrite_stdout();
    fgets(message, BUFFER_SIZE, stdin);
    str_trim_lf(message, BUFFER_SIZE);

    if (strcmp(message, "exit") == 0) {
			break;
    } else {
      sprintf(buffer, "%s: %s\n", name, message);
      send(sockFD, buffer, strlen(buffer), 0);
    }

		bzero(message, BUFFER_SIZE);
    bzero(buffer, BUFFER_SIZE + 32);
  }
  catch_ctrl_c_and_exit(2);

}

void recv_message_handler()
{
    char message[BUFFER_SIZE] = {};

    while(1)
    {

        int recieve = recv(sockFD,message,BUFFER_SIZE,0);
        if(recieve > 0)
        {
            printf("%s",message);
            str_overwrite_stdout();
        }else if(recieve == 0)
        {
        break;
        }
        else
        {
         // -1 Error

        }

        memset(message,0,sizeof(message));

    }

}


int main(int argc, char **argv)
{

    if(argc != 2)
    {
        printf("Usage: %s <port>\n",argv[0]);
        return EXIT_FAILURE;
    }

    char* ip = "127.0.0.1";
    int port = atoi(argv[1]);


    struct sockaddr_in server_addr;

    //SIGNALTE die gefangen werden strg+c und exit
    signal(SIGINT, catch_ctrl_c_and_exit);


    //Socket Settings
    sockFD = socket(AF_INET,SOCK_STREAM,0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    //Connect to Server

    int conn = connect(sockFD,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if(conn == -1){

        printf("Failed to Connect To server Try again !\n");
        return EXIT_FAILURE;

    }else
        printf("Please Enter your Name: ");
        fgets(name, 32, stdin);
        str_trim_lf(name, strlen(name));

        if(strlen(name) > MAX_NAMELEN || strlen(name) < 2)
        {
            printf("The name must be longer than 2 characters and less than %d\n",MAX_NAMELEN);
            return EXIT_FAILURE;
        }


        //First thing to give the Server is the name

        send(sockFD,name,MAX_NAMELEN,0);

        pthread_t send_msg_thread;

        if(pthread_create(&send_msg_thread,NULL,(void* ) send_message_handler,NULL ) != 0)
        {
            perror("ERROR: Failed to Create send_message_handler!\n");
            return EXIT_FAILURE;
        }

        pthread_t recv_msg_thread;
        if(pthread_create(&recv_msg_thread,NULL,(void* ) recv_message_handler,NULL ) != 0)
        {
            perror("ERROR: Failed to Create send_message_handler!\n");
            return EXIT_FAILURE;
        }

        while(1)
        {
        if(flag)
        {
        printf("Thnx for being in our Server\n");
        break;
        }

        }

        close(sockFD);



    return EXIT_SUCCESS;
}


