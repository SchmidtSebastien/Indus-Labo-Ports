#ifndef PORT_H
#define PORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "Common.h"
#include "Ressources.h"

void create_processes(int nb_docks, char* port_name);
void init_ressources(Semaphore* mutex_boat, Semaphore* sem_port, Semaphore* mutex_dep, Semaphore* mutex_dock, Semaphore* mutex_arr, Shm* shm_dep, Shm* shm_arr, Shm* shm_dock, Shm* shm_boat, char* port_name, int nb_docks, int nb_boats);
void print_boat(char* port_name, int boat_index, char* msg);
Boat get_actual_boat(boat_p position, char* port, int nb_boats, Shm shm_boat);
Boat get_waiting_boat(char* port, int nb_boats, Shm shm_boat);
void book_dock(Semaphore mutex_dock, Shm shm_dock, Boat boat, int nb_docks);

#endif /* PORT_H */
