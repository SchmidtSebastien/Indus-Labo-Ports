#include "Ressources.h"

/**************/
/* Semaphores */
/**************/
void open_sem(Semaphore *sem)
{
	if (sem->oflag == 0)
	{
		if((sem->p_sem = sem_open(sem->semname, sem->oflag)) == SEM_FAILED)
		{
			printf("Tried to open  : %s\n", sem->semname);
			perror("Semaphore opening error");
			exit(errno);
		}
	}
	else
	{
		if ((sem->p_sem = sem_open(sem->semname, sem->oflag, sem->mode, sem->value)) == SEM_FAILED)
		{
			printf("Tried to open  : %s\n", sem->semname);
			perror("Semaphore opening error");
			exit(errno);
		}
	}
}

void wait_sem(Semaphore sem)
{
    if (sem_wait(sem.p_sem) == -1)
    {
        perror("sem_wait");
        exit(errno);
    }

}

void signal_sem(Semaphore sem)
{
    if (sem_post(sem.p_sem) == -1)
    {
        perror("sem_post");
        exit(errno);
    }
}

void close_sem(Semaphore sem)
{
    if (sem_close(sem.p_sem) == -1)
    {
        perror("sem_close");
        exit(errno);
    }
}

void destroy_sem(Semaphore sem)
{
	close_sem(sem);

    // Le semaphore nomme n'existe plus dans l'OS
    if (sem_unlink(sem.semname) == -1)
    {
        perror("sem_unlink");
        exit(errno);
    }
}

/**************/
/*    Signal  */
/**************/
/*void armerSignal(struct sigaction *act, int signal)
{
    if(sigaction(signal, act, 0) == -1)
	{
        perror("sigaction error");
        exit(errno);
    }
}*/

/**************/
/*    Shm     */
/**************/
void open_shm(Shm *memPartagee)
{
	// Creation de la memoire partagee
	if ((memPartagee->fdShm = shm_open(memPartagee->shmName, memPartagee->mode, 0)) == -1)
	{
		perror("Memoire partagee initialisation");
		exit(errno);
	}

    // Modification des privilèges d'accès
    if (fchmod(memPartagee->fdShm, (mode_t) S_IRWXG | S_IRWXU | S_IRWXO) == -1 )
    {
        perror("Modification droits d'accès");
        exit(errno);
    }

	// Modification de la taille de la memoire partagee
	if(ftruncate(memPartagee->fdShm, memPartagee->sizeofShm) == -1)
	{		
		perror("Modification taille de la memoire partagee");
		exit(errno);
	}
}

void mapping_shm(Shm *memPartagee, size_t size)
{
	// Mapping de la memoire partagee
	if ((memPartagee->pShm = mmap(0, size, (PROT_WRITE | PROT_READ), MAP_SHARED, memPartagee->fdShm, 0)) == MAP_FAILED)
	{
        perror("Mapping");
        exit(errno);
    }
}

void close_shm(Shm memPartagee)
{
	if (shm_unlink(memPartagee.shmName) == -1)
	{
		perror("Unlink shm");
		exit(errno);
	}
}

/**************/
/*    Mq      */
/**************/
void open_mq(MessageQueue *mq, struct mq_attr* attr)
{
	if ((mq->mq = mq_open(mq->name, mq->oflag, mq->mode, attr)) == -1)
	{
		perror("Ouverture MessageQueue");
		exit(errno);
	}
}

void close_mq(MessageQueue mq)
{
	if (mq_close(mq.mq) == -1)
	{
		perror("Close MessageQueue");
		exit(errno);
	}

	if (mq_unlink(mq.name) == -1)
	{
		perror("Unlink MessageQueue");
		exit(errno);
	}
}

void send_mq(mqd_t msgq,const char * message,int taille,unsigned int msg_prio)
{
	if (mq_send(msgq,message,taille,msg_prio) == -1)
	{
		perror("Envoie messageQueue");
		exit(errno);
	}
}

int recv_mq(mqd_t msgq,char * message,int taille,unsigned int *msg_prio)
{	
	return mq_receive(msgq,message,taille, msg_prio);
}











