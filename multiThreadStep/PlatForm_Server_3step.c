#include <stdio.h>
#include <dlfcn.h>
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
#include <sys/time.h>
#include <pthread.h> //멀티스레드 사용 헤더
#include <signal.h> //시그널 사용 헤더
#include <sys/sem.h>//세마포어 사용 헤더

#define MAXLINE 512


int pid, startTime;
int *arrPid;
int tid = 0;

pthread_mutex_t m_lock;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void handler(int sig){
    pid_t pid;
    
    if((pid = waitpid( -1, NULL, 0))<0){
        printf("waitpid error\n");
        exit(0);
    }
    printf("시그널함수로 프로세스끝넴 %d\n",pid);
}

union semu{
    int val;
};//세마포어 구조체

void *(*result)();

void * checkT(void* data){

    //printf("프로그램 실행됬음 tid %d\n",tid);
    int p = tid;

    time_t stime;
    time(&stime);
    result();
    time_t etime;
    time(&etime);
    
    //printf("tid %d 걸린시간 %ld\n",p,etime-stime);
    pthread_mutex_lock(&m_lock);
    //뮤텍스 설정
    arrPid[p] = etime-stime;
    pthread_mutex_unlock(&m_lock);
    return 0;
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
    
    ////세마포어 설정///////////////////////////
    int semid;
    union semu sem_union;
    
    struct sembuf semopen = {0 , -1, SEM_UNDO};
    struct sembuf semclose = {0, 1, SEM_UNDO};
    
    semid = shmget((key_t)12345, 1, IPC_CREAT|0666);
    if(semid == -1){
        perror("shmget failed : ");
        exit(0);
    }
    
    sem_union.val = 1;
    
    if( -1 == semctl(semid,0,SETVAL, sem_union)){
        perror("semctl failed : ");
        exit(0);
    }
    
    
    
    ////////////////////////////////
    
    
    
    
    //////////////////////////////////////
    //////////공유메모리 사용 ////////////
    /////////////////////////////////////

    int shmid;
    void *shared_memory = NULL;
    
    int count = 1000000;
    key_t key = 12345678;
    const int shareSize = sizeof(int) * count;
    
    shmid = shmget(key,shareSize ,0666|IPC_CREAT);
    if( shmid == -1){
        perror("shmget failed : ");
        exit(0);
    }
    
    shared_memory = shmat(shmid, NULL, 0);
    if(shared_memory == (void *)-1){
        perror("shmat failed : ");
        exit(0);
    }
    
    arrPid = (int*)shared_memory;
    ///////////////////////////////
    //////////////////////////////////////
    if(signal(SIGCHLD, handler) == SIG_ERR){
        printf("signal error");
        exit(0);
    }
    
    while(1){
        
        
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
                
            case 1 :
            {
                
                //printf("프로그램 복사를 하고 실행했음.\n");
                int n;
                int dest_fd;
                char fileName;
                char copyName[MAXLINE];
                
                
                ////////////////////////
                ///////////1////////////
                ///////파일사이즈 소켓수신///
                
                if (read(newsockfd,buffer,MAXLINE)< 0)
                    error("ERROR reading from socket");
                
                int fileSize = atoi(buffer);
                //printf("파일사이즈 : %d\n",fileSize);
                //파일 사이즈를 받음
                

                
                bzero(buffer,MAXLINE);
                
                
                ////////////////////////
                ///////////2////////////
                ///////파일이름 소켓수신////
                
                if(read(newsockfd,buffer,MAXLINE)<0){//파일이름 소켓으로 받기
                    error("Error not recieve file name\n");
                    exit(1);
                }
                
                sprintf(copyName,"%s","1"); //소켓으로 파일이름 받은것 저장
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
                //printf("[%s] 파일 저장완료\n",copyName);
                close(dest_fd);
                
                //파일 저장하기
                bzero(buffer,MAXLINE);

                /////////////////////////////
                ///////쓰레드 생성 및 so파일 오픈//
                
                void *handle = NULL;

                handle = dlopen(copyName,RTLD_NOW);
                result = dlsym(handle,"timer");
                //so 파일 받아서 함수실행

                tid++;
                pthread_create(&thread_id,NULL,checkT,NULL);
                pthread_detach(thread_id);

                sprintf(buffer,"%d",tid);
                
                
                ////////////////////////
                ///////////4////////////
                ///////pid 소켓전송///////
                if ((write(newsockfd,buffer,MAXLINE)) < 0) error("ERROR writing to socket");
                bzero(buffer,MAXLINE);
                
                
                break;
            }
        
    case 0:
        {
                //status 일때 수행하는 코드들
                ////////////////////////
                ///////////1////////////
                ///////파일사이즈 소켓수신///
                printf("status 결과를 전송함.\n");
                if (read(newsockfd,buffer,MAXLINE)< 0)
                    error("ERROR reading from socket");

            
            
                int ch_pid = atoi(buffer);
                //printf("받은 pid : %d\n",ch_pid);
                //pid를 전송 받음

            
                if(arrPid[ch_pid-1] == -1){
                    //프로그램이 돌고 있지 않을때
                    bzero(buffer,MAXLINE);
                    sprintf(buffer,"%d",-1);
                    write(newsockfd,buffer,MAXLINE); //프로그램이 돌지 않음을 보냄
                    bzero(buffer,MAXLINE);

                    
                }else if(arrPid[ch_pid-1] == 0){
                    //프로그램이 돌고 있을때
                    bzero(buffer,MAXLINE);
                    sprintf(buffer,"%d",0);
                    write(newsockfd,buffer,MAXLINE); //프로그램이 돌고 있음을 보냄
                    bzero(buffer,MAXLINE);
                    
                }else if(arrPid[ch_pid-1] > 0){
                    //프로그램이 종료되었을때
                    bzero(buffer,MAXLINE);
                    sprintf(buffer,"%d",arrPid[ch_pid-1]);
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
    return 0; /* we never get here */
    
}
