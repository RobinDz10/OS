#include <xinu.h>
#include <string.h>
#include <fs.h>


shellcmd xsh_run(int nargs, char *args[]){
 
    if((nargs == 1) || (strncmp(args[1], "list", 4) == 0 || (strncmp(args[1], "fstest", 6) != 0 && strncmp(args[1], "run", 3) != 0 && strncmp(args[1], "prodcons", 8) != 0
     && strncmp(args[1], "hello", 5) != 0))){
        printf("fstest\n");
        printf("futest\n");
        printf("hello\n");
        printf("list\n");
        printf("prodcons\n");
        printf("prodcons_bb\n");
        printf("tscdf\n");
        printf("tscdf_fq\n");
    }
    
    args++;
    nargs--;
    
    if(strncmp(args[0], "fstest", 6) == 0){
        fstest(nargs, args);
    }

    return 0;
}