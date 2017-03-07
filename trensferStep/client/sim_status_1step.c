#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#define MAXLINE 512

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) //simsubmit portnumber file path
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[MAXLINE];
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[1]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        error("ERROR connecting");
    }
    
    bzero(buffer,MAXLINE);
    sprintf(buffer,"%d",0);
    write(sockfd,buffer,MAXLINE); //처음 status 라는 정보를 보냄
    bzero(buffer,MAXLINE);
    
    write(sockfd,argv[2],sizeof(argv[1])); //pid를 보냄
    bzero(buffer,MAXLINE);
    
    read(sockfd,buffer,MAXLINE);
        //printf("%s is option....\n",buffer);
    int option = atoi(buffer);
      //  printf("%d is option....\n",option);
    
    
    int pid = atoi(argv[2]);
    
    if(option == 0){
        printf("Job %d is running....\n",pid);
        exit(0);
    }else if(option > 0){
        printf("Job %d was done\n", pid );
        printf("Job %d took %d seconds \n",pid,option);
        printf("Results from Fob %d\n", pid );
        exit(0);
    }else if(option == -1){
        printf("Job %d is not running....\n",pid);
        exit(0);
    }
    
    ////////////////////////////
    
    return 0;
}
