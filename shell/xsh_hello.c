/* xsh_hello.c - xsh_hello */

#include <xinu.h>
#include <string.h>
#include <stdio.h>

/* ----------------------------------------
 * xsh_hello - print a message says "hello"
 * ----------------------------------------
 */

shellcmd xsh_hello(int nargs, char *args[]){
    // /* Check if arguments are too many */
    // if(nargs > 2){
    //     fprintf(stderr, "%s: too many arguments\n", args[0]);
    //     return 1;
    // }

    // /* Check if arguments are too few */ 
    // if(nargs < 2){
    //     fprintf(stderr, "%s: too few arguments\n", args[0]);
    //     return 1;
    // }

    if(nargs != 2){
        printf("Syntax: run hello name\n");
        return 1;
    }

    printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);
    return 0;
}
