#include <xinu.h>
#include <future.h>
#include<stdio.h>
#include<string.h>

future_t *future_alloc(future_mode_t mode, uint size, uint nelem) {
    intmask mask;
    mask = disable();

    // TODO: write your code here

    struct future_t *future_t_p = (struct future_t *) getmem(size * sizeof(struct future_t *));
    future_t_p->size = size;
    future_t_p->mode = mode;
    future_t_p->data = (char *) getmem(size);


    if (mode == FUTURE_SHARED) {
        qid16 qid = newqueue();
    }

    restore(mask);
    return future_t_p;
}

// TODO: write your code here for future_free, future_get and future_set
syscall future_free(future_t *f) {
    kill(f->pid);

    if (f->mode == FUTURE_SHARED) {
        int queue_size = 0;
        while (!isempty(f->get_queue)) {
            kill(dequeue(f->get_queue));
            queue_size++;
        }
        delqueue(f->get_queue);
        freemem(f->get_queue, sizeof(qid16) * queue_size);
    }

    freemem(f->data, sizeof(f->data));
    freemem((char *) f, f->size * sizeof(f));
    return OK;
}


syscall future_get(future_t *f, char *out) {
    if (f->state == FUTURE_EMPTY) {
        f->state = FUTURE_WAITING;
        f->pid = getpid();
        if (f->mode == FUTURE_SHARED) {
            enqueue(getpid(), f->get_queue);
        }
        suspend(getpid());
        resched();
    } else if (f->state == FUTURE_WAITING) {
        if (f->mode == FUTURE_EXCLUSIVE)
            return SYSERR;
        else if (f->mode == FUTURE_SHARED) {
            enqueue(getpid(), f->get_queue);
        }
    }
    if (f->state == FUTURE_READY) {
        f->state = FUTURE_EMPTY;
        memcpy(out, f->data, f->size);
    }
    return OK;
}


syscall future_set(future_t *f, char *in) {
    if (f->state == FUTURE_READY) {
        return SYSERR;
    } else if (f->state == FUTURE_WAITING) {
        memcpy(f->data, in, sizeof(in));
        f->state = FUTURE_READY;
        resume(f->pid);
        if (f->mode == FUTURE_SHARED) {
            while (!isempty(f->get_queue)) {
                resume(dequeue(f->get_queue));
            }
        }
    } else {
        memcpy(f->data, in, sizeof(in));
        f->state = FUTURE_READY;
    }
    return OK;
}












