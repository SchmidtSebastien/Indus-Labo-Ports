#ifndef GESTION_H
#define GESTION_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

//#include "Ressources.h"
#include "Common.h"

#define PROP_FILE	"../Config.cfg"

char* getProp(const char *fileName, const char *propName);
void init_ressources(int nb_boats);

Semaphore mutex_boat;

Shm shm_boat;

#endif /* GESTION_H */	