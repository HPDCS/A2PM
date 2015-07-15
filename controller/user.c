#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<pthread.h>

char ip[16];

int user(void *argv)
{
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];

    while(1){   
    //Create socket
    	sock = socket(AF_INET , SOCK_STREAM , 0);
    	if (sock == -1)
    	{
        	printf("Could not create socket");
    	}
    	//puts("Socket created");
     
    	server.sin_addr.s_addr = inet_addr(ip);
    	server.sin_family = AF_INET;
    	server.sin_port = htons( 8080);
    	//Connect to remote server
    	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0){
        	perror("connect failed. Error");
        	return 1;
    	}
     
        //Send some data
        /*if( send(sock , message , 16 , 0) < 0)
        {
            puts("Send failed");
            return 1;
        }*/
        //Receive a reply from the server
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            puts("recv failed");
            break;
        }
        printf("%s\n", server_reply);
        printf(".");     
    	fflush(stdout);
	close(sock);
    }

    return 0;
}

int main(int argc, char * argv[]){
	int n_th = atoi(argv[2]);
	int i;
	pthread_t tid;
	strcpy(ip,argv[1]);
	for(i = 0; i < n_th; i++){
		pthread_create(&tid,NULL,user,(void *)argv);
	}
	printf("Type to close the user session!\n");
	getchar();
	return 0;
}
