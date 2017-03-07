
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h> //멀티스레드 사용 헤더
#define MAXLINE 512


int pid, startTime;
int pidArray[100000][2];

pthread_mutex_t m_lock;


void * thread_checkProcess(void *args){//전달받을 인자 구조체 포인터
    
    while(1){
        int status;
        
        ///////////////////////////
        /////////시간 재기////////////
        ////////////////////////////

        int pid = wait(&status);
        sleep(1);
        if(pid != -1){
            pthread_mutex_lock(&m_lock);
            //전역 변수 설정시 뮤텍스 설정
            time_t t2;
            time(&t2);
            //종료된 시간
            
            pidArray[pid-1][0] = (int)t2-pidArray[pid-1][0] - 1;
            pidArray[pid-1][1] = 1;
            //프로그램 종료 되었을때
            printf("%d 프로세스 종료됬으며, 공유메모리에 결과저장 완료 \n걸린시간 : %d \n",pid,pidArray[pid-1][0]);
            pthread_mutex_unlock(&m_lock);
            //뮤텍스 종료 부분
        }
    }
    return 0;
}


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    pthread_t thread_id;
    struct sockaddr_in serv_addr, cli_addr;
    
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0){
        error("ERROR on binding");
    }else{
        printf("connection is made\n");
    }
    
    if((listen(sockfd,5))<0){
        error("Error on listening\n");
    }
    clilen = sizeof(cli_addr);
    
    //////////////뮤텍스////////////
    if(pthread_mutex_init(&m_lock,NULL) !=0){
        perror("Mutex Init failure");
        return 1;
    }
    
    ///////////////////////////////
    
    pthread_create(&thread_id, NULL, thread_checkProcess,(void *)NULL);
    pthread_detach(thread_id);
    /////////////쓰레드는 미리 시작 시켜놓음
    
    
    ///////////////////////////
    for(int i = 0; i < 100000; i++){
        pidArray[i][1] = -1;
    }
    
    ///////////배열 초기화 ///////////
    
    while(1){
        
        ///소켓 열리자마자 쓰레드 진행
        
        
        printf("파일 전송을 기다리는중.....\n");
        newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
        //accept 소켓을 받아들이는 부분
        /////////////////////////
        
        
        char buffer[MAXLINE];
        bzero(buffer,MAXLINE);
        
        
        
        if (read(newsockfd,buffer,MAXLINE)< 0)
            error("ERROR reading from socket");
        //////////0 . 상태 체크 /////////////
        //////////1 . 파일 입력 /////////////
        
        int option = 0;
        option = atoi(buffer);
        
        switch(option){
                
            case 1://프로그램을 복사 및 실행
            {
                printf("프로그램 복사를 하고 실행했음.\n");
                int n;
                int dest_fd;
                char fileName;
                
                ////////////////////////
                ///////////1////////////
                ///////파일사이즈 소켓수신///
                
                if (read(newsockfd,buffer,MAXLINE)< 0)
                    error("ERROR reading from socket");
                
                int fileSize = atoi(buffer);
                printf("파일사이즈 : %d\n",fileSize);
                //파일 사이즈를 받음
                
                char copyName[MAXLINE];
                
                bzero(buffer,MAXLINE);
                
                
                ////////////////////////
                ///////////2////////////
                ///////파일이름 소켓수신////
                
                if(read(newsockfd,buffer,MAXLINE)<0){//파일이름 소켓으로 받기
                    error("Error not recieve file name\n");
                    exit(1);
                }
                
                sprintf(copyName,"%s",buffer); //소켓으로 파일이름 받은것 저장
                bzero(buffer,MAXLINE);
                
                if(!(dest_fd = open(copyName, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO))){
                    printf("error : it can't make a file\n");
                    exit(1);
                }
                
                char *copy_buf;
                copy_buf = (char*)malloc(fileSize);//버퍼 자동할당
                
                
                
                ////////////////////////
                ///////////3////////////
                ///////파일내용 소켓수신////
                
                if((read(newsockfd,copy_buf,fileSize)) < 0)
                    printf("error : 버퍼 복사 에러\n");
                //버퍼에 파일내용을 복사
                
                if((write(dest_fd,copy_buf,fileSize)) < 0)
                    printf("error : 파일 복사 에러\n");
                //파일에 복사해온 내용을 저장함
                
                free(copy_buf);
                printf("[%s] 파일 저장완료\n",copyName);
                close(dest_fd);
                
                //파일 저장하기
                bzero(buffer,MAXLINE);
                
                //////////////////////////////////////
                
                pid = fork();
                time_t t0;
                time(&t0);
                pidArray[pid-1][0] = t0;
                pidArray[pid-1][1] = 0;
                //프로그램 시작한 시간 저장
                int or_pid = pid;
                
                sprintf(buffer,"%d",pid);
                
                
                ////////////////////////
                ///////////4////////////
                ///////pid 소켓전송///////
                if ((write(newsockfd,buffer,MAXLINE)) < 0) error("ERROR writing to socket");
                bzero(buffer,MAXLINE);
                
                if (pid < 0)
                    error("ERROR on fork");
                if (pid == 0)  {
                    //자식 장소
                        printf("자식 프로세스에서 프로그램 실행 \n");
                            if( execl(copyName,"Timer",NULL) < 0 ){
                                printf("실행 실패 \n");
                                pidArray[pid-1][1] = -1;
                                exit(0);
                            }
                }else{
                
                }
            
                break;
            }
                
        
            case 0://프로세스의 상태를 확인 후 전송
            {
                //sim_status 일때 수행하는 코드들
            
                ////////////////////////
                ///////////1////////////
                ///////파일사이즈 소켓수신///
                printf("status 결과를 전송함.\n");
                if (read(newsockfd,buffer,MAXLINE)< 0)
                    error("ERROR reading from socket");

            
            
                int ch_pid = atoi(buffer);
                //pid를 전송 받음
            
                if(pidArray[ch_pid-1][1] == -1){
                    //프로그램이 돌고 있지 않을때
                    bzero(buffer,MAXLINE);
                    sprintf(buffer,"%d",-1);
                    write(newsockfd,buffer,MAXLINE); //프로그램이 돌지 않음을 보냄
                    bzero(buffer,MAXLINE);

                    
                }else if(pidArray[ch_pid-1][1] == 0){
                    //프로그램이 돌고 있을때
                    bzero(buffer,MAXLINE);
                    sprintf(buffer,"%d",0);
                    write(newsockfd,buffer,MAXLINE); //프로그램이 돌고 있음을 보냄
                    bzero(buffer,MAXLINE);
                    
                }else if(pidArray[ch_pid-1][1] == 1){
                    //프로그램이 종료되었을때
                    bzero(buffer,MAXLINE);
                    sprintf(buffer,"%d",pidArray[pid-1][0]);
                    write(newsockfd,buffer,MAXLINE); //프로그램이 돌아간 시간을 보냄
                    bzero(buffer,MAXLINE);
                }
                
                
                
                bzero(buffer,MAXLINE);
          
            
                break;
                
                
                
            }
        }
        
        
    }
    
    close(newsockfd);
    close(sockfd);
    return 0;
    
}
