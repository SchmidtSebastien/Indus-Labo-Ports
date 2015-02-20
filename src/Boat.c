#include "Boat.h"

int main(int argc, char* argv[])
{
	Semaphore mutex_boat;
	Semaphore sem_port;
	Semaphore mutex_dep;
	Semaphore sem_dock;
	Semaphore mutex_arr;
	Semaphore mutex_sync;

	Shm shm_dep;
	Shm shm_arr;
	Shm shm_boat;
	char* ports_name[] = {"Douvre", "Calais", "Dunkerque"};
	int index;
	int cpt_arr;
	int cpt_dep;
	sscanf(argv[1], "%d", &index);
	int stop = 0;
	Boat boat;
	struct sigaction act;
	char* msg = malloc(sizeof(msg));
	act.sa_handler = handler;

	srand(getpid());

    /*if(sigaction(SIGUSR1, &act, 0) == -1)
    {
        perror("sigaction error");
        exit(errno);
    }

    if(sigaction(SIGUSR2, &act, 0) == -1)
    {
        perror("sigaction error");
        exit(errno);
    }*/

	// MUTEX_SYNC
	mutex_sync.oflag = (O_CREAT | O_RDWR);
    mutex_sync.mode  = 0644;
    mutex_sync.value = 1;
    sprintf(mutex_sync.semname,"%s%d", MUTEX_SYNC, index);
	
	sem_unlink(mutex_sync.semname);
	open_sem(&mutex_sync);

	// MUTEX_BATEAU
	mutex_boat.oflag = O_RDWR;
    mutex_boat.mode  = 0644;
    mutex_boat.value = 1;
    strcpy(mutex_boat.semname, MUTEX_BOAT);

	open_sem(&mutex_boat);

	shm_boat.sizeofShm = sizeof(Boat) * 6;
	shm_boat.mode = O_RDWR;
	strcpy(shm_boat.shmName, SHM_BOAT);

	open_shm(&shm_boat);
	mapping_shm(&shm_boat, sizeof(Boat) * 6);

	// Placement de l'état par défaut du bateau
	boat.pid = getpid();
	boat.index = index;
	boat.position = SEA;
	boat.direction = UNDEFINED;
	boat.state_changed = 0;
	
	wait_sem(mutex_boat);
	memcpy(shm_boat.pShm + (index * sizeof(Boat)), &boat, sizeof(Boat));
	signal_sem(mutex_boat);
	
	while (!stop)
	{
		// Lecture de l'état du bateau
		wait_sem(mutex_boat);
		memcpy(&boat, shm_boat.pShm + (index * sizeof(Boat)), sizeof(Boat));
		signal_sem(mutex_boat);

		char* port_name;

		switch(boat.position)
		{
			case SEA:
				// Premier voyage 
				if (boat.direction == UNDEFINED)
					boat.direction = rand() % 3 + 1;
				// Les bateaux viennent d'un port
				else
				{
					if (boat.direction == CALAIS || boat.direction == DUNKERQUE)
						boat.direction = DOVER;
					else
						boat.direction =  rand() % (3 - 2 + 1) + 2;
				}

				port_name = ports_name[boat.direction - 1];	

				// Simulation de la traversée
				int duration = rand() % (30 - 15 + 1) + 15;
				printf("Traversée de bateau %d pendant %d seconde\n", index, duration);
				sleep(duration);

				boat.state_changed = 1;
				boat.position = ENTERS_PORT;
				break;

			case ENTERS_PORT:
			{	
				// Récupération des ressources du port
				sem_port.oflag = O_RDWR;
				sem_port.mode  = 0644;
				sem_port.value = 0;
				sprintf(sem_port.semname,"%s%s", SEM_PORT, port_name);

				// MUTEX_DEP
				mutex_dep.oflag = O_RDWR;
				mutex_dep.mode  = 0644;
				mutex_dep.value = 1;
				sprintf(mutex_dep.semname,"%s%s", MUTEX_DEP, port_name);	

				// MUTEX_ARR
				mutex_arr.oflag = O_RDWR;
				mutex_arr.mode  = 0644;
				mutex_arr.value = 1;
				sprintf(mutex_arr.semname,"%s%s", MUTEX_ARR, port_name);

				// SHM_DEP
				shm_dep.sizeofShm = sizeof(int);
				shm_dep.mode = O_RDWR;
				sprintf(shm_dep.shmName,"%s%s", SHM_DEP, port_name);

				// SHM_ARR
				shm_arr.sizeofShm = sizeof(int);
				shm_arr.mode = O_RDWR;
				sprintf(shm_arr.shmName,"%s%s", SHM_ARR, port_name);

				open_sem(&sem_port);
				open_sem(&mutex_dep);
				open_sem(&mutex_arr);

				open_shm(&shm_dep);
				mapping_shm(&shm_dep, sizeof(int));

				open_shm(&shm_arr);
				mapping_shm(&shm_arr, sizeof(int));

				wait_sem(mutex_arr);
				memcpy(&cpt_arr, shm_arr.pShm, sizeof(int));
				cpt_arr++;
				memcpy(shm_arr.pShm, &cpt_arr, sizeof(int));
				signal_sem(mutex_arr);
				signal_sem(sem_port);

				sprintf(msg, "Devant l'entrée de %s", port_name);
				print_boat(index, msg);

				printf("Attente du signal\n");
				//pause();
				wait_sem(mutex_sync);

				sprintf(msg, "Entree dans le port de %s", port_name);
				print_boat(index, msg);
				boat.position = DOCK;
				break;
			}
			case DOCK:
				sprintf(msg, "Embarquement/Debarquement");
				print_boat(index, msg);
				boat.position = LEAVES_PORT;
				break;
			case LEAVES_PORT:

				wait_sem(mutex_dep);
				memcpy(&cpt_dep, shm_dep.pShm, sizeof(int));
				cpt_dep++;
				memcpy(shm_dep.pShm, &cpt_dep, sizeof(int));
				signal_sem(mutex_dep);
				signal_sem(sem_port);

				sprintf(msg, "Devant la sortie de %s", port_name);
				print_boat(index, msg);

				//pause();
				wait_sem(mutex_sync);

				sprintf(msg, "Sortie du port de %s", port_name);
				print_boat(index, msg);

				// Fermeture des ressources du port
				close_sem(sem_port);
				close_sem(mutex_dep);
				close_sem(mutex_arr);
				boat.position = SEA;
				break;
		}

		// Copie de la structure
		wait_sem(mutex_boat);
		memcpy(shm_boat.pShm + (index * sizeof(Boat)), &boat, sizeof(Boat));
		signal_sem(mutex_boat);
	}
	return EXIT_SUCCESS;
}

void print_boat(int index, char* msg)
{
	char* color[] = {"\x1B[31m", "\x1B[32m", "\x1B[33m", "\x1B[34m", "\x1B[35m", "\x1B[36m"};
	char* reset = "\033[0m";

	printf("Bateau %d> %s%s%s\n", index, color[index], msg, reset);
}

void handler(int sig)
{
	printf("Signal recu %d\n", sig);
}
