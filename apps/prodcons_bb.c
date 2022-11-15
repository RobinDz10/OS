#include <xinu.h>
#include <prodcons_bb.h>

int arr_q[5] = {-1, -1, -1, -1, -1};   // globally int array
int head;       // globally write index
int tail;       // globally read index
int runCount;   // globally max read times

void prodcons_bb(int nargs, char* args[]){
    runCount = 0;
    // Initialize arr_q with all default value -1
    // If arr_q[head] == -1, can write
    // If arr_q[tail] == -1, can NOT read

    // Check if there are 4 input parametes
    if(nargs != 5){
        printf("Syntax: run prodcons_bb <# of producer processes> <# of consumer processes> <# of iterations the producer runs> <# of iterations the consumer runs>\n");
        return;
    }
    // Check if all 4 input parameters are integers
    char *s1 = args[1];
    char *s2 = args[2];
    char *s3 = args[3];
    char *s4 = args[4];
    // num1 : number of producer
    // num2 : number of consumer
    // num3 : number of write per producer
    // num4 : number of read per consumer
    int num1 = 0, num2 = 0, num3 = 0, num4 = 0;
    for(int i = 0; i < strlen(s1); i++){
        if(!(s1[i] >= '0' && s1[i] <= '9')){
            printf("Syntax: run prodcons_bb <# of producer processes> <# of consumer processes> <# of iterations the producer runs> <# of iterations the consumer runs>\n");
            return;
        }
        else{
            num1 = num1 * 10 + (s1[i] - '0');
        }
    }
    for(int i = 0; i < strlen(s2); i++){
        if(!(s2[i] >= '0' && s2[i] <= '9')){
            printf("Syntax: run prodcons_bb <# of producer processes> <# of consumer processes> <# of iterations the producer runs> <# of iterations the consumer runs>\n");
            return;
        }
        else{
            num2 = num2 * 10 + (s2[i] - '0');
        }
    }
    for(int i = 0; i < strlen(s3); i++){
        if(!(s3[i] >= '0' && s3[i] <= '9')){
            printf("Syntax: run prodcons_bb <# of producer processes> <# of consumer processes> <# of iterations the producer runs> <# of iterations the consumer runs>\n");
            return;
        }
        else{
            num3 = num3 * 10 + (s3[i] - '0');
        }
    }
    for(int i = 0; i < strlen(s4); i++){
        if(!(s4[i] >= '0' && s4[i] <= '9')){
            printf("Syntax: run prodcons_bb <# of producer processes> <# of consumer processes> <# of iterations the producer runs> <# of iterations the consumer runs>\n");
            return;
        }
        else{
            num4 = num4 * 10 + (s4[i] - '0');
        }
    }
    // Check if iteration mismatch
    if(num1 * num3 != num2 * num4){
        printf("Iteration Mismatch Error: the number of producer(s) iteration does not match the consumer(s) iteration\n");
        return;
    }
    sid32 produced, consumed, flag2, mutex;
    consumed = semcreate(0);
    produced = semcreate(1);
    flag2 = semcreate(0);
    mutex = semcreate(1);
    int maxBound = num2 * num4;
    for(int i = 0; i < num1; i++){
        resume(create(producer_bb, 1024, 20, "producer_bb", 7, consumed, produced, flag2, mutex, i, num3, maxBound));
    }
    for(int i = 0; i < num2; i++){
        resume(create(consumer_bb, 1024, 20, "consumer_bb", 7, consumed, produced, flag2, mutex, i, num4, maxBound));
    }
    wait(flag2);
    return;
}
