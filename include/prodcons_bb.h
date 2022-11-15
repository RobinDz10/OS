#include <xinu.h>
// declare globally shared array
// declare globally shared semaphores
// declare globally shared read and write indices
extern int arr_q[5];    // globally shared array
extern int head;        // globally shared write index
extern int tail;        // globally shared read index
extern int runCount;

// function prototypes

void producer_bb(sid32 produced, sid32 consumed, sid32 flag2, sid32 mutex, int id, int count, int maxBound);
void consumer_bb(sid32 produced, sid32 consumed, sid32 flag2, sid32 mutex, int id, int count, int maxBound);