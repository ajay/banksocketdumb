#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "server.h"
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>

#define reset   "\x1b[0m"
#define red     "\x1b[31m"
#define green   "\x1b[32m"
#define yellow  "\x1b[33m"

static int acc = 5;

char* doprocessing (char *buffer, int sock, int shmid)
{
	// acc++;




	int n;
	bzero(buffer, 256);
	n = read(sock, buffer, 255);

	char sub[5];
	strncpy(sub, buffer, 4);
	sub[4] = '\0';
	n = write(sock, sub,4);

	if (n < 0)
	{
		perror(red "ERROR reading from socket: " reset);
		exit(1);
	}
	if (strcmp(sub, "exit") == 0)
	{
		printf("GOTTA EXIT \n");
	}

	if (n < 0)
	{
		perror(red "ERROR writing to socket: " reset);
		exit(1);
	}

	printf("received: %s\n", buffer);
	printf("acc is %d\n", acc);




	sharedMemory *shm = (sharedMemory *)shmat(shmid, NULL, 0);


	printf("locking sem, pid = %d\n", getpid());
	sem_wait(&shm->accountsArray[acc].lock);

	printf("gay ass memory spot account name %d: %s\n", acc, shm->accountsArray[acc].name);
	printf("gay ass memory spot balance %d: %f\n", acc, shm->accountsArray[acc].balance);

	bzero(buffer, 256);
	n = read(sock, buffer, 255);

	puts("unlocking sem");
	sem_post(&shm->accountsArray[acc].lock);



	return buffer;
}

int main(int argc, char *argv[])
{
	if (argc != 1)
	{
		printf(red "wyd bitch?\n" reset);
		exit(1);
	}

	//creating shared memory
	int shmid;
	key_t key;
	sharedMemory *shm;

	int totalSize = sizeof(sharedMemory);

	printf("sizeof sharedMemory: %d\n", totalSize);

	key = 2214;

	shmid = shmget(key, totalSize, IPC_CREAT | IPC_EXCL | 0666); // call to create shared memory

	if(shmid < 0)
	{
		shmid = shmget(key, totalSize, 0666);
		shmctl(shmid, IPC_RMID, NULL);
		shmid = shmget(key, totalSize, IPC_CREAT | IPC_EXCL | 0666);
		printf("%d\n", shmid);
		// exit(1);
	}

	 // call to create shared memory
	shm = (sharedMemory*)shmat(shmid, NULL, 0);

	printf("Yoooo size of shm: %ld\n", sizeof(shm));
	memset((sharedMemory *)shm, 0, totalSize);
 	shmdt(shm);

	if(shm == (sharedMemory *) -1)
	{
		perror("shmat");
		exit(1);
	}

	sharedMemory *shmPointer = (sharedMemory *)shmat(shmid, NULL, 0);


	int j;
	for(j = 0; j < 20; j++)
	{
		sem_t locker;
		printf("writing j = %d\n", j);
		char title[80];
		sprintf(title, "account %d", j);
		strcpy(shm->accountsArray[j].name, title);
		shmPointer->accountsArray[j].balance = rand();
		shmPointer->accountsArray[j].lock = locker;
		sem_init(&shmPointer->accountsArray[j].lock, 1, 1);
	}








	int sockfd;
	int newsockfd;
	int portno;
	socklen_t clilen;

	// char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	// int n;
	int pid;

	// Call socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		perror(red "ERROR opening socket: " reset);
		exit(1);
	}

	// Initialize socket
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = 5001;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	// Bind address
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror(red "ERROR on binding: " reset);
		exit(1);
	}

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	while (1)
	{
		printf("I am pid %d listening\n", pid);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (newsockfd < 0)
		{
			perror(red "ERROR on accept: " reset);
			exit(1);
		}

		pid = fork();

		// Forking error
		if (pid < 0)
		{
			perror(red "ERROR on fork: " reset);
			exit(1);
		}

		if (pid == 0) // Child process
		{
			char buffer[256];

			while (strcmp(buffer, "exit") != 0)
			{
				strcpy(buffer, doprocessing(buffer, newsockfd, shmid));
			}
			printf("I am pid %d exiting\n", getpid());
			close(sockfd);
			exit(0);
		}

		else // Parent process
		{
			close(newsockfd);
		}
	}
}