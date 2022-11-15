#include <xinu.h>
#include <future.h>
#include <stddef.h>
#include <future_prodcons.h>

sid32 print_sem;

uint future_prod(future_t *fut, char *value) {
    int status;
    wait(print_sem);
    printf("Producing %d\n", *value);
    signal(print_sem);
    status = (int) future_set(fut, value);
    if (status < 1) {
        wait(print_sem);
        printf("future_set failed\n");
        signal(print_sem);
        return -1;
    }
    return OK;
}

uint future_cons(future_t *fut) {
    char *i = NULL;
    int status;
    status = (int) future_get(fut, i);
    if (status < 1) {
        wait(print_sem);
        printf("future_get failed\n");
        signal(print_sem);
        return -1;
    }
    wait(print_sem);
    printf("Consumed %d\n", *i);
    signal(print_sem);
    return OK;
}

