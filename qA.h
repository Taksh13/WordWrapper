#include <pthread.h>

#ifndef QSIZE
#define QSIZE 8
#endif

typedef struct
{
	char *data[QSIZE];
	unsigned count;
	unsigned head;
	int open;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
	pthread_cond_t write_ready;
} qA_t;

int startA(qA_t *Q);
int destroyA(qA_t *Q);
int enqueueA(qA_t *Q, char *item);
int dequeueA(qA_t *Q, char **item);
int qcloseA(qA_t *Q);
void printA(qA_t *Q);