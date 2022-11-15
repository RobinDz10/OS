

#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<a7_stream_proc.h>
#include<a7_init_stream_input.h>
#include <tscdf.h>

#define MAXLIST 100000

int work_queue_depth, num_streams, time_window, output_time;
int32 pcport;

int stream_proc(int nargs, char *args[]) {

    ulong secs, msecs, time;
    secs = clktime;
    msecs = clkticks;


    // TODO: Create consumer processes and initialize streams

    char usage[] = "Usage: run tscdf -s <num_streams> -w <work_queue_depth> -t <time_window> -o <output_time>\n";

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
    struct stream stream_lst[MAXLIST];

    /**
     * This time, do not parse data stream, only use iteration number to start consumer threads
     */
    for (int i = 0; i < num_streams; i++) {

        stream_lst[i].spaces = semcreate(work_queue_depth);// modify in parse2
        stream_lst[i].items = semcreate(0);
        stream_lst[i].mutex = semcreate(1);
        stream_lst[i].head = 0;
        stream_lst[i].tail = 0;
        stream_lst[i].queue=(struct data_element *) getmem(work_queue_depth*sizeof(struct data_element));

        // create consumer processes
        //        resume(create((void *) xsh_hello, 4096, 20, "xsh_hello", 2, nargs, args));
//        printf("%d\n",i);
        resume(create((void *) stream_consumer, 4096, 20, "stream_consumer", 2, i, &stream_lst[i]));

    }
//    printf("done\n");


    /**
     * This time, parse data
     */
    for (int i = 0; i < number_inputs; i++) {

        char *a = (char *) stream_input[i];
        int st = atoi(a);
        while (*a++ != '\t');
        int ts = atoi(a);
        while (*a++ != '\t');
        int v = atoi(a);
//        printf("%d, %d, %d\n",st,ts,v);

        // wait allocated mutex
        wait(stream_lst[st].spaces);
        wait(stream_lst[st].mutex);

        // assign value
        // using head to locate offset in a big queue
        (*(stream_lst[st].queue + stream_lst[st].head)).time=ts;
        (*(stream_lst[st].queue + stream_lst[st].head)).value=v;

//        printf("input value: %d, %d\n",(*(stream_lst[i].queue + stream_lst[i].head)).time, (*(stream_lst[i].queue + stream_lst[i].head)).value);
        // head offset ++
        stream_lst[st].head = (stream_lst[st].head + 1) % work_queue_depth;

        // signal
        signal(stream_lst[st].mutex);
        signal(stream_lst[st].items);

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


void stream_consumer(int32 id, struct stream *str) {

    // TODO: Print the current id and pid

    // TODO: Consume all values from the work queue of the corresponding stream
    int count=0;
    struct tscdf *tc = tscdf_init(time_window);


    kprintf("stream_consumer id:%d (pid:%d)\n", id, getpid());

    while (1) {

        wait(str->items);
        wait(str->mutex);

        count++;


        // use tail to locate READ offset
        int32 time_val=(*(str->queue + str->tail)).time;
        int32 value_val=(*(str->queue + str->tail)).value;

        if(time_val==0 && value_val==0)
            break;

//        kprintf("Read data: %d, %d\n",time_val ,value_val);
        // move tail forward
        str->tail = (str->tail + 1) % work_queue_depth;

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

        signal(str->mutex);
        signal(str->spaces);
    }
    kprintf("stream_consumer exiting\n");
    ptsend(pcport, getpid());
    return;
}












