#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>

#ifndef SERVER_H
#define SERVER_H

typedef struct Account
{
	char name[100];
	float balance;
	sem_t lock;
	bool inSession;
} Account;


typedef struct sharedMemory
{
	Account accounts[20];
	sem_t bankLock;
	bool bankInUse;
} sharedMemory;

#endif