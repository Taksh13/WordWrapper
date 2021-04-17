#include <pthread.h>

typedef struct
{
	char **data;
	unsigned count;
	unsigned head;
	unsigned activeThreads;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
} qB_t;

int startB(qB_t *Q);
int destroyB(qB_t *Q);
int enqueueB(qB_t *Q, char *item);
int dequeueB(qB_t *Q, char **item);
void printB(qB_t *Q);