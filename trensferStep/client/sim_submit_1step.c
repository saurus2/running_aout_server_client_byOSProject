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
    }else{
        printf("connection is made \n");
    }
    
    bzero(buffer,MAXLINE);
    sprintf(buffer,"%d",1);
    write(sockfd,buffer,MAXLINE); //처음 submit이라는 정보를 보냄
    bzero(buffer,MAXLINE);
    
    //파일 경로 입력 받기
    char *f_name;
    //char f_buffer[MAXLINE];
    //bzero(f_buffer,MAXLINE);
    //printf("파일 경로 입력 : ");
    //fgets(f_buffer,MAXLINE,stdin);
    //printf("이거슨 %s",argv[3]);
    //sprintf(f_buffer,"%s",argv[3]);
    //f_buffer[strlen(f_buffer)-1] = '\0';
    
    //파일 경로를 buffer로 저장
    FILE *pf;
    
    pf = fopen(argv[2],"rb");
    //바이너리 모드로 열어서 실제 파일 사이즈와 같게 만듦
    if(pf==NULL){
        printf("error : file not found\n");
        exit(1);
    }
    
    int send_fsize;
    
    fseek(pf,0,SEEK_END);
    send_fsize = ftell(pf);
    fseek(pf,0,SEEK_SET);
    sprintf(buffer,"%d",send_fsize);
    fclose(pf);
    //파일 사이즈를 버퍼에 저장해서 보냄
    
    ////////////////////////
    ///////////1////////////
    ///////파일사이즈 소켓전송///
    if(!(write(sockfd,buffer,MAXLINE))){ //파일 사이즈 보낼때 버퍼 사이즈를 작게 잡은거 수정
        printf("not receive file size\n");
        exit(1);
    }
    
    bzero(buffer,MAXLINE);
    
    ////////////////////////
    ///////////2////////////
    ///////파일이름 소켓전송////
    
    write(sockfd,argv[2],sizeof(argv[2]));//파일이름 전송
    
    
    int source_fd;
    if(!(source_fd = open(argv[2],O_RDONLY))){ //파일을 열기
        printf("error : file not found\n");
        exit(1);
    }
    
    char *copy_buf;
    copy_buf = (char*)malloc(send_fsize);
    
    if((read(source_fd,copy_buf,send_fsize)) < 0)
        error("Error copying to buffer\n"); //버퍼에 파일내용 복사하기
    
    ////////////////////////
    ///////////3////////////
    ///////파일내용 소켓전송////
    
    if ((write(sockfd,copy_buf,send_fsize)) < 0){//소켓으로 버퍼 보내기
        error("ERROR writing to socket\n");
    }else{
        printf("simulator is sent\n");
    }
    
    free(copy_buf);
    bzero(buffer,MAXLINE);
    //bzero(f_buffer,MAXLINE);
    
    ////////////////////////
    ///////////4////////////
    ///////pid 소켓수신///////
    
    if ((read(sockfd,buffer,MAXLINE))< 0)
        error("ERROR reading from socket\n");
    printf("Job id is %s\n",buffer);
    bzero(buffer,MAXLINE);
    
    close(sockfd);
    return 0;
}
