#include <xinu.h>
#include <prodcons.h>


int n;      // Definition for global variable 'n'
/* Now global variable n is accessible by all the processes i.e. consume and produce */

shellcmd xsh_prodcons(int nargs, char *args[]){
    // Argument verifications and validations
    int count = 200;    // local varible to hold count, Original default: 2000, change to 200 (Assignment 3)
    n = 0;
    if(nargs != 2){
        printf("Syntax: run prodcons [counter]\n");
        return (0);
    }

    // Convert chars to integer
    char *s = args[1];
    int num = 0;
    for(int i = 0; i < strlen(s); i++){
        if(!(s[i] >= '0' && s[i] <= '9')){
            fprintf(stderr, "%s: Invalid argument!\n", args[0]); // Check if argument contain chars other than ('0' ~ '9')
            return (0);
        }
        else{
            num = num * 10 + (s[i] - '0');
        }
    }
    count = num; // Pass the assign value to count
/*
    // create the process producer and consumer and put them in ready queue.
    
    // Look at the definations of function create and resume in the system folder for reference.
*/
    sid32 produced, consumed, flag1;
    consumed = semcreate(0);
    produced = semcreate(1);
    flag1 = semcreate(0);
    resume(create(producer, 1024, 20, "producer", 4, consumed, produced, flag1, count));
    resume(create(consumer, 1024, 20, "consumer", 4, consumed, produced, flag1, count));
    wait(flag1);
    return 0;
}