#include <xinu.h>
#include <future.h>

uint32 get_heap_size(void) {

    int i;                 /* Index into process table  */
    uint32 stack = 0;      /* Total used stack memory  */
    uint32 kfree = 0;      /* Total free memory    */
    struct memblk *block;  /* Ptr to memory block    */

    /* Calculate amount of allocated stack memory */
    /*  Skip the NULL process since it has a private stack */
    for (i = 1; i < NPROC; i++) {
        if (proctab[i].prstate != PR_FREE) {
            stack += (uint32)proctab[i].prstklen;
        }
    }

    /* Calculate the amount of memory on the free list */
    for (block = memlist.mnext; block != NULL; block = block->mnext) {
        kfree += block->mlength;
    }

    return kfree - stack;
}

int future_free_test(int nargs, char *args[]) {

    uint32 baseline = get_heap_size();
    uint32 compare;

    // Allocate FUTURE_EXCLUSIVE
    future_t *f_exclusive;
    if ((f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1)) == (future_t *) SYSERR) {
        printf("future exclusive creation failed\n");
        return OK;
    }

    compare = baseline - get_heap_size();
    if (compare < sizeof(future_t)) {
        printf("future exclusive creation did not allocate enough memory\n");
        return OK;
    }

    printf("future exclusive created\n");


    // Free FUTURE_EXCLUSIVE
    if (future_free(f_exclusive) != OK) {
        printf("future exclusive free failed\n");
        return OK;
    }

    compare = baseline - get_heap_size();
    if (compare != 0) {
        printf("future exclusive free did not free enough memory\n");
        return OK;
    }

    printf("future exclusive freed\n");


    // Allocate FUTURE_SHARED
    future_t *f_shared;
    if ((f_shared = future_alloc(FUTURE_SHARED, sizeof(int), 1)) == (future_t *) SYSERR) {
        printf("future shared creation failed\n");
        return OK;
    }

    compare = baseline - get_heap_size();
    if (compare < sizeof(future_t)) {
        printf("future shared creation did not allocate enough memory\n");
        return OK;
    }

    printf("future shared created\n");


    // Free FUTURE_SHARED
    if (future_free(f_shared) != OK) {
        printf("future shared free failed\n");
        return OK;
    }

    compare = baseline - get_heap_size();
    if (compare != 0) {
        printf("future shared free did not free enough memory\n");
        return OK;
    }

    printf("future shared freed\n");

    return OK;
}
