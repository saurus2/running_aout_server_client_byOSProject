#include <stdio.h>
#include <time.h>

int timer(){
    int result;
    clock_t start, finish;
    double duration=0.0;
    
    while(1){
        start = clock();
        while(1){
            
            if(clock() > start + (15*CLOCKS_PER_SEC)){// 68초가 지나면 탈출
                finish = clock();
                break;
            }
        }
        
        duration = (int)(finish-start)/CLOCKS_PER_SEC; // 사직과 끝의 차이로 걸린 초를 구함
        result = duration;
        printf("took %d seconds\n", result);
        
        break;
        }
    return 0;
}


