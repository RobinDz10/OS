#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

void consumer(sid32 consumed, sid32 produced, sid32 flag1, int count) {
    extern int n;
    // TODO: implement the following:
    // - Iterates from 0 to count (count including)
    //   - reading the value of the global variable 'n' each time
    //   - print consumed value (the value of 'n'), e.g. "consumed : 8"
    for(int i = 0; i <= count; ++i){
        wait(produced);
        printf("consumed : %d\n", n);
        signal(consumed);
    }
}


void consumer_bb(sid32 produced, sid32 consumed, sid32 flag2, sid32 mutex, int id, int count, int maxBound){
    // TODO: implement the following:
    // - Iterate from 0 to count (count excluding)
    //   - read the next available value from the global array `arr_q`
    //   - print consumer id (starting from 0) and read value as:
    //     "name : consumer_X, read : X"
    extern int arr_q[5];
    extern int head;
    extern int tail;
    extern int runCount;

    for(int i = 0; i < count; i++){
        wait(mutex);
        if(arr_q[tail] != -1){
            printf("name : consumer_%d, read : %d\n", id, arr_q[tail]);
            runCount++;
            arr_q[tail] = -1;
            tail++;
            if(tail == 5){
                tail = 0;
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