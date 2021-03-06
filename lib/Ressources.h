#ifndef RESSOURCES_H
#define RESSOURCES_H

#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#define SHM_NAME_LENGTH 50
#define SEM_NAME_LENGTH 50
#define MQ_NAME_LENGTH  50

typedef struct Shm_t
{
	int fdShm;
	caddr_t pShm;
	mode_t 	mode;
	char shmName[SHM_NAME_LENGTH];
	size_t sizeofShm;
} Shm;

typedef struct Semaphore_t
{
	sem_t*  p_sem;
	int     oflag;
	mode_t  mode;
	char    semname[SEM_NAME_LENGTH];
	unsigned int value;
} Semaphore;

typedef struct MessageQueue_t
{
	mqd_t mq;
	int oflag;
	mode_t mode;
	char name[MQ_NAME_LENGTH];
} MessageQueue;

// Semaphore
void open_sem(Semaphore *sem);
void wait_sem(Semaphore sem);
void signal_sem(Semaphore sem);
void close_sem(Semaphore sem);
void destroy_sem(Semaphore sem);

// Shm
void open_shm(Shm *memPartagee);
void mapping_shm(Shm *memPartagee, size_t size);
void close_shm(Shm memPartagee);

// Mq
void open_mq(MessageQueue *mq, struct mq_attr* attr);
void close_mq(MessageQueue mq);
void send_mq(mqd_t msgq,const char * message,int taille,unsigned int msg_prio);
int recv_mq(mqd_t msgq,char * message,int taille,unsigned int *msg_prio);

#endif /* RESSOURCES_H */
