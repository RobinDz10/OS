

#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<a7_stream_proc.h>
#include<a8_stream_proc.h>
#include<a8_init_stream_input.h>
#include <tscdf.h>
#include<future.h>

#define MAXLIST 100000

int work_queue_depth, num_streams, time_window, output_time;
int32 pcport;

int stream_proc_futures(int nargs, char *args[]) {

    ulong secs, msecs, time;
    secs = clktime;
    msecs = clkticks;


    // TODO: Create consumer processes and initialize streams

    char usage[] = "run tscdf_fq -s <num_streams> -w <work_queue_depth> -t <time_window> -o <output_time>\n";

    int i;
    char *ch, c;
    if (nargs != 9) {
        printf("%s", usage);
        return SYSERR;
    } else {
        i = nargs - 1;
        while (i > 0) {
            ch = args[i - 1];
            c = *(++ch);

            switch (c) {
                case 's':
                    num_streams = atoi(args[i]);
                    break;

                case 'w':
                    work_queue_depth = atoi(args[i]);
                    break;

                case 't':
                    time_window = atoi(args[i]);
                    break;

                case 'o':
                    output_time = atoi(args[i]);
                    break;

                default:
                    printf("%s", usage);
                    return SYSERR;
            }

            i -= 2;
        }
    }



    ptinit(MAXLIST);
    pcport = ptcreate(num_streams);
    if (pcport == -1) {
        panic("have a look at ptcreate\n");
        return SYSERR;
    }


    // Use `i` as the stream id.
    //     struct stream stream_lst[MAXLIST];

    future_t **futures=(struct future_t **)getmem(sizeof(struct future_t) * num_streams);

    /**
     * This time, do not parse data stream, only use iteration number to start consumer threads
     */
    for (int i = 0; i < num_streams; i++) {

//        stream_lst[i].spaces = semcreate(work_queue_depth);// modify in parse2
//        stream_lst[i].items = semcreate(0);
//        stream_lst[i].mutex = semcreate(1);
//        stream_lst[i].head = 0;
//        stream_lst[i].tail = 0;
//        stream_lst[i].queue=(struct data_element *) getmem(work_queue_depth*sizeof(struct data_element));

        *(futures+i)=future_alloc(FUTURE_QUEUE, sizeof(struct data_element), work_queue_depth);


        // create consumer processes
        //        resume(create((void *) xsh_hello, 4096, 20, "xsh_hello", 2, nargs, args));
//        printf("%d\n",i);
        resume(create((void *) stream_consumer_future, 4096, 20, "stream_consumer_future", 2, i, *(futures+i)));

    }
//    printf("done\n");


    /**
     * This time, parse data
     */
    for (int i = 0; i < number_inputs2; i++) {

        char *a = (char *) stream_input2[i];
        int st = atoi(a);
        while (*a++ != '\t');
        int ts = atoi(a);
        while (*a++ != '\t');
        int v = atoi(a);
//        printf("%d, %d, %d\n",st,ts,v);
        struct data_element *da=(struct data_element*)getmem(sizeof(struct data_element));
        da->time=ts;
        da->value=v;

        future_set(*(futures+st), da);
        // this is important to prevent panic
        freemem(da,sizeof(*da));


        // wait allocated mutex

    }

    // TODO: Parse input header file data and populate work queue

    // TODO: Join all launched consumer processes

    // TODO: Measure the time of this entire function and report it at the end


    for (i = 0; i < num_streams; i++) {
        printf("process %d exited\n", ptrecv(pcport));
    }

// Code you want to time

    time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
    printf("time in ms: %u\n", time);

    return OK;
}


void stream_consumer_future(int32 id, future_t *f) {


    // TODO: Print the current id and pid

    // TODO: Consume all values from the work queue of the corresponding stream
    int count=0;
    struct tscdf *tc = tscdf_init(time_window);


    kprintf("stream_consumer_future id:%d (pid:%d)\n", id, getpid());

    while (1) {
        struct data_element *da=(struct data_element*)getmem(sizeof(struct data_element));
        future_get(f, da);

        count++;

        // use tail to locate READ offset
        int32 time_val=da->time;
        int32 value_val=da->value;

        if(time_val==0 && value_val==0)
            break;

//        kprintf("Read data: %d, %d\n",time_val ,value_val);

        tscdf_update(tc, time_val, value_val);

        if (count == output_time) {
            int32 *qarray = tscdf_quartiles(tc);
            char *output=(char *) getmem(MAXLIST*sizeof(char));

            if (qarray == NULL) {
                kprintf("tscdf_quartiles returned NULL\n");
                continue;
            }

            sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
            kprintf("%s\n", output);
            freemem((char *) qarray, (6*sizeof(int32)));

            count = 0;
        }

    }
    kprintf("stream_consumer_future exiting\n");
    ptsend(pcport, getpid());
    return;
}












