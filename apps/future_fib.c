#include <xinu.h>
#include <stdlib.h>
#include <future.h>

future_t **fibfut;

int zero = 0;
int one = 1;
int two = 2;

int ffib(int n) {

    int minus1 = 0;
    int minus2 = 0;
    int this = 0;

    if (n == 0) {
        future_set(fibfut[0], (char*) &zero);
        return OK;
    }

    if (n == 1) {
        future_set(fibfut[1], (char*) &one);
        return OK;
    }

    int status = (int) future_get(fibfut[n-2], (char*) &minus2);

    if (status < 1) {
        printf("future_get failed\n");
        return -1;
    }

    status = (int) future_get(fibfut[n-1], (char*) &minus1);

    if (status < 1) {
        printf("future_get failed\n");
        return -1;
    }

    this = minus1 + minus2;

    future_set(fibfut[n], (char*) &this);

    return(0);

}

int future_fib(int nargs, char *args[]) {
    int fib = -1, i;

    if (nargs != 3) {
        printf("Syntax: run futest [-pc [g ...] [s VALUE ...]|-f NUMBER][--free]\n");
        return(SYSERR);
    }

    fib = atoi(args[2]);

    if (fib > -1) {
        int final_fib;
        int future_flags = FUTURE_SHARED;

        // create the array of future pointers
        if ((fibfut = (future_t **)getmem(sizeof(future_t *) * (fib + 1)))
            == (future_t **) SYSERR) {
            printf("getmem failed\n");
            return(SYSERR);
        }

        // get futures for the future array
        for (i=0; i <= fib; i++) {
            if((fibfut[i] = future_alloc(future_flags, sizeof(int), 1)) == (future_t *) SYSERR) {
                printf("future_alloc failed\n");
                return(SYSERR);
            }
        }

        // spawn fib threads and get final value
        for (i=0; i <= fib; i++) {
            char name[10];
            sprintf(name, "ffib_%d", i);
            resume(create(ffib, 1024, 20, name, 1, i));
        }
        future_get(fibfut[fib], (char*) &final_fib);

        // clean up
        for (i=0; i <= fib; i++) {
            future_free(fibfut[i]);
        }
        freemem((char *)fibfut, sizeof(future_t *) * (fib + 1));

        printf("Nth Fibonacci value for N=%d is %d\n", fib, final_fib);
        return(OK);
    }
    return(SYSERR);
}
