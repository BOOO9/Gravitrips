#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048
#define NAME_LENGTH 32


static _Atomic unsigned int cli_count = 0;
static int uid = 5;

typedef struct
{
    struct sockaddr_in address;
    int sockFd;
    int uid;
    char name[NAME_LENGTH];

} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;


void print_IPAddr(struct sockaddr_in addr)
{
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24
          );

}


void str_overwrite_stdout()
{
    printf("\r%s", "> ");
    fflush(stdout);

}

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


// Add Client Queue
void queue_add(client_t *cl)
{
    pthread_mutex_lock(&client_mutex);

    for(int i = 0 ; i < MAX_CLIENTS ; i++)
    {
        if(!clients[i])
        {
            clients[i] = cl;
            break;
        }
    }

    pthread_mutex_unlock(&client_mutex);

}




//Remove Client Queue

void remove_queue(int uid)
{
    pthread_mutex_lock(&client_mutex);

    for(int i = 0 ; i < MAX_CLIENTS ; i++)
    {
        if(clients[i])
        {
            if(clients[i]->uid == uid)
            {
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&client_mutex);

}

//Send Messages
void send_message(char* message , int uid)
{
    pthread_mutex_lock(&client_mutex);

    for(int i = 0 ; i < MAX_CLIENTS ; i++)
    {
        if(clients[i])
        {
            if(clients[i]->uid != uid)
            {
            if(write(clients[i]->sockFd,message,strlen(message)) < 0)
            {
            perror("ERROR: Failed to write to File Descriptor\n");
            break;
            }

            }
        }
    }


    pthread_mutex_unlock(&client_mutex);
}



void *client_handler(void *arg)
 {
    char buff_out[BUFFER_SIZE];
    char name[NAME_LENGTH];
    int leave_flag = 0 ;

    cli_count++;

    client_t *client = (client_t*)arg;

    //Check Add Name

    if(recv(client->sockFd,name,NAME_LENGTH,0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LENGTH - 1)
    {
        printf("User didn't enter the name\n");
        leave_flag = 1;
    }else
    {
        strcpy(client->name,name);
        sprintf(buff_out, "%s has joined the Chat room\n",client->name);
        printf("%s",buff_out);
        send_message(buff_out,client->uid);

    }

    bzero(buff_out,BUFFER_SIZE);

    while(1)
    {
        if(leave_flag)
        {
            break;
        }

        int recieve = recv(client->sockFd,buff_out,BUFFER_SIZE,0);
        if(recieve > 0 )
        {
            if(strlen(buff_out) > 0)
            {
                send_message(buff_out,client->uid);
                str_trim_lf(buff_out,strlen(buff_out));
                printf("%s: %s",client->name,buff_out);

            }
        }else if(recieve == 0 || strcmp(buff_out,"exit" == 0))
        {
        sprintf(buff_out,"%s has left the Chatroom\n",client->name);
        printf("%s",buff_out);
        send_message(buff_out,client->uid);
        leave_flag = 1;
        }else
        {
            perror("Error -1");
            leave_flag = 1;
        }
        bzero(buff_out,BUFFER_SIZE);
    }


    //Wenn Leave Flag 1 User Disconnected oder User Left

    close(client->sockFd);
    remove_queue(client->uid);
    free(client);

    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
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

    int option = 1;
    int listenFd = 0, connFd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;

    pthread_t tid;


    //Socket Settings
    listenFd = socket(AF_INET,SOCK_STREAM,0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    //SIGNAL
    signal(SIGPIPE,SIG_IGN);

    //SOCKEt OPTIONS

    if(setsockopt(listenFd, SOL_SOCKET,( SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
    {
        printf("ERROR: Socket Options Failed...\n");
        return EXIT_FAILURE;
    }


    //BIND
    if(bind(listenFd,(struct sockaddr*)&serv_addr,sizeof(serv_addr) ) < 0)
    {
        printf("Socket Binding Failed");
        return EXIT_FAILURE;
    }


    //LISTEN

    if(listen(listenFd,10) < 0)
    {
        printf("ERROR: Listening Failed");
        return EXIT_FAILURE;
    }


    printf("WELCOME TO THE CHAT\n");


    while(1)
    {
        socklen_t clientLen = sizeof(client_addr);

        connFd = accept(listenFd,(struct sockaddr*)&client_addr,&clientLen);


        if((cli_count + 1 ) == MAX_CLIENTS)
        {
            printf("Chatroom is full / Connection Rejected\n");
            print_IPAddr(client_addr);
            printf("%d\n", client_addr.sin_port);
            close(connFd);
            continue;

        }

        //Client Settings

        client_t *client = (client_t*)malloc(sizeof(client_t));
        client->address =  client_addr;
        client->sockFd = connFd;
        client->uid = uid++;

        //Add Clients to queue
        queue_add(client);

        pthread_create(&tid,NULL,&client_handler,(void*)client);


        //Reduziere CPU Usage

        sleep(1);


    }



    return EXIT_SUCCESS;
}

