#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

void producer(sid32 consumed, sid32 produced, sid32 flag1, int count) {
    extern int n;
    // TODO: implement the following:
    // - Iterates from 0 to count (count including)
    //   - setting the value of the global variable 'n' each time
    //   - print produced value (new value of 'n'), e.g.: "produced : 8"
    for(int i = 0; i <= count; i++){
        wait(consumed);
        printf("produced : %d\n", n);
        n++;
        signal(produced);
    }
    signal(flag1);
}


void producer_bb(sid32 produced, sid32 consumed, sid32 flag2, sid32 mutex, int id, int count, int maxBound){
    // TODO: implement the following:
    // - Iterate from 0 to count (count excluding)
    //   - add iteration value to the global array `arr_q`
    //   - print producer id (starting from 0) and written value as:
    //     "name : producer_X, write : X"
    extern int arr_q[5];
    extern int head;
    extern int tail;
    
    for(int i = 0; i < count; i++){
        wait(mutex);
        if(arr_q[head] == -1){
            arr_q[head] = i;
            printf("name : producer_%d, write : %d\n", id, arr_q[head]);
            head++;
            if(head == 5){
                head = 0;
            }
        }
        else{
            i--;
        }
        signal(mutex);
    }
    if(runCount == maxBound){
        signal(flag2);
    }
}