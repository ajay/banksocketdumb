#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include "client_handler.c"

#ifndef SERVER_H
#define SERVER_H

typedef struct Account
{
	char name[100];
	float balance;
	sem_t lock;
} Account;


typedef struct sharedMemory
{
	Account accountsArray[20];
	sem_t bankLock;
} sharedMemory;

#endif