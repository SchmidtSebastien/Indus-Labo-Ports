#include "Boat.h"

int main(int argc, char* argv[])
{
	Semaphore mutex_boat;
	Semaphore sem_port;
	Semaphore mutex_dep;
	Semaphore sem_dock;
	Semaphore mutex_arr;
	Semaphore mutex_sync;
	Semaphore mutex_dock;

	Shm shm_dock;
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
	struct mq_attr attr;
	struct sigaction act;
	char* msg = malloc(sizeof(msg));

	srand(getpid());

	// Initialisation des ressources
	init_ressources(&mutex_sync, &mutex_boat, &shm_boat, index);

	// Placement de l'état par défaut du bateau
	boat.pid = getpid();
	boat.index = index;
	boat.position = SEA;
	boat.direction = UNDEFINED;
	boat.waiting = 0;
	
	attr.mq_curmsgs = 0;
	attr.mq_flags = 0;
	attr.mq_maxmsg = MQ_MAXSIZE;
	attr.mq_msgsize = MQ_MSGSIZE;

	// MQ CAMIONS
	boat.mq1.oflag = (O_CREAT | O_EXCL | O_RDWR ) | O_NONBLOCK;
	boat.mq1.mode  = 0644;
	sprintf(boat.mq1.name,"%s%d", MQ_TRUCKS, index);

	// MQ VOITURES ET CAMIONNETTES
	boat.mq2.oflag = (O_CREAT | O_EXCL | O_RDWR ) | O_NONBLOCK;
	boat.mq2.mode  = 0644;
	sprintf(boat.mq2.name,"%s%d", MQ_CARS_VANS, index);

	mq_unlink(boat.mq1.name);
	mq_unlink(boat.mq2.name);
	open_mq(&boat.mq1, &attr);
	open_mq(&boat.mq2, &attr);

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
			/* ***********************************************/
			/*						SEA						 */
			/* ***********************************************/
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
				int duration = rand() % 5 + 10;
				sprintf(msg, "Traversée vers %s (%d secondes)", port_name, duration);
				print_boat(index, msg);
				sleep(duration);

				boat.position = ENTERS_PORT;
				break;
			/* ***********************************************/
			/*					ENTERS_PORT					 */
			/* ***********************************************/
			case ENTERS_PORT:
			{	
				// Récupération des ressources du port
				open_port_ressources(&sem_port, &mutex_dep, &mutex_arr, &shm_dep, &shm_arr, port_name);

				wait_sem(mutex_arr);
				memcpy(&cpt_arr, shm_arr.pShm, sizeof(int));
				cpt_arr++;
				memcpy(shm_arr.pShm, &cpt_arr, sizeof(int));
				signal_sem(mutex_arr);
				signal_sem(sem_port);

				sprintf(msg, "Devant l'entrée de %s", port_name);
				print_boat(index, msg);

				wait_sem(mutex_sync);
				//printf("### BOAT %d ENTERS_PORT [%s]\n", index, port_name);
				sprintf(msg, "Entree dans le port de %s", port_name);
				print_boat(index, msg);
				boat.position = DOCK;
				break;
			}
			/* ***********************************************/
			/*						DOCK					 */
			/* ***********************************************/
			case DOCK:
			{
				int nb_docks = (strcmp(port_name, "Douvre") == 0) ? 3 : 2;
				
				// Création ressouces du quai
				open_dock_ressources(&mutex_dock, &shm_dock, port_name, nb_docks);

				// Recherche de l'id du quai
				int i;
				int dock_index;
				int found = 0;
				Dock dock;
				wait_sem(mutex_dock);
				for (i = 0; i < nb_docks && !found; i++)
				{
					memcpy(&dock, shm_dock.pShm + (i * sizeof(Dock)), sizeof(Dock));
					printf("#[%s]# BOAT_INDEX [%d] == DOCK.BOAT_INDEX [%d] ?\n", port_name, index, dock.boat_index);
					if (dock.boat_index == index)
					{
						dock_index = dock.index;
						printf("Index trouvé : %d\n", dock_index);
						found = 1;
					}
				}
				signal_sem(mutex_dock);

				// Création de la sémaphore correspondante
				sem_dock.oflag = O_RDWR;
				sem_dock.mode  = 0644;
				sem_dock.value = 0;
				sprintf(sem_dock.semname, "%s%s%d", SEM_DOCK, port_name, dock_index);
				sprintf(msg, "Sem : %s", sem_dock.semname);
				print_boat(index, msg);
				open_sem(&sem_dock);

				// Debloque le quai
				signal_sem(sem_dock);

				// Autorisation pour la sortie du port
				wait_sem(mutex_sync);

				sprintf(msg, "Quitte le quai");
				print_boat(index, msg);
				boat.position = LEAVES_PORT;
				break;
			}
			/* ***********************************************/
			/*					LEAVES_PORT					 */
			/* ***********************************************/
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

void init_ressources(Semaphore* mutex_sync, Semaphore* mutex_boat, Shm* shm_boat, int index)
{
	// MUTEX_SYNC
	mutex_sync->oflag = (O_CREAT | O_RDWR);
	mutex_sync->mode  = 0644;
	mutex_sync->value = 0;
	sprintf(mutex_sync->semname,"%s%d", MUTEX_SYNC, index);

	sem_unlink(mutex_sync->semname);
	open_sem(mutex_sync);

	// MUTEX_BATEAU
	mutex_boat->oflag = O_RDWR;
	mutex_boat->mode  = 0644;
	mutex_boat->value = 1;
	strcpy(mutex_boat->semname, MUTEX_BOAT);

	open_sem(mutex_boat);

	shm_boat->sizeofShm = sizeof(Boat) * 6;
	shm_boat->mode = O_RDWR;
	strcpy(shm_boat->shmName, SHM_BOAT);

	open_shm(shm_boat);
	mapping_shm(shm_boat, sizeof(Boat) * 6);
}

void open_port_ressources(Semaphore* sem_port, Semaphore* mutex_dep, Semaphore* mutex_arr, Shm* shm_dep, Shm* shm_arr, char* port_name)
{
	sem_port->oflag = O_RDWR;
	sem_port->mode  = 0644;
	sem_port->value = 0;
	sprintf(sem_port->semname,"%s%s", SEM_PORT, port_name);

	// MUTEX_DEP
	mutex_dep->oflag = O_RDWR;
	mutex_dep->mode  = 0644;
	mutex_dep->value = 1;
	sprintf(mutex_dep->semname,"%s%s", MUTEX_DEP, port_name);	

	// MUTEX_ARR
	mutex_arr->oflag = O_RDWR;
	mutex_arr->mode  = 0644;
	mutex_arr->value = 1;
	sprintf(mutex_arr->semname,"%s%s", MUTEX_ARR, port_name);

	// SHM_DEP
	shm_dep->sizeofShm = sizeof(int);
	shm_dep->mode = O_RDWR;
	sprintf(shm_dep->shmName,"%s%s", SHM_DEP, port_name);

	// SHM_ARR
	shm_arr->sizeofShm = sizeof(int);
	shm_arr->mode = O_RDWR;
	sprintf(shm_arr->shmName,"%s%s", SHM_ARR, port_name);

	open_sem(sem_port);
	open_sem(mutex_dep);
	open_sem(mutex_arr);

	open_shm(shm_dep);
	mapping_shm(shm_dep, sizeof(int));

	open_shm(shm_arr);
	mapping_shm(shm_arr, sizeof(int));
}

void open_dock_ressources(Semaphore* mutex_dock, Shm* shm_dock, char* port_name, int nb_docks)
{
	mutex_dock->oflag = O_RDWR;
	mutex_dock->mode  = 0644;
	mutex_dock->value = 1;
	sprintf(mutex_dock->semname,"%s%s", MUTEX_DOCK, port_name);

	open_sem(mutex_dock);

	// SHM_DOCK
	shm_dock->sizeofShm = sizeof(Dock) * nb_docks;
	shm_dock->mode = O_RDWR;
	sprintf(shm_dock->shmName,"%s%s", SHM_DOCK, port_name);

	open_shm(shm_dock);
	mapping_shm(shm_dock, sizeof(Dock) * nb_docks);
}

void print_boat(int index, char* msg)
{
	char* color[] = {"\x1B[31m", "\x1B[32m", "\x1B[33m", "\x1B[34m", "\x1B[35m", "\x1B[36m"};
	char* reset = "\033[0m";

	printf("Bateau %d> %s%s%s\n", index, color[index], msg, reset);
}
