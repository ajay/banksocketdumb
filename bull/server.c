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
#include <signal.h>

#define reset	"\x1b[0m"
#define red 	"\x1b[31m"	// Errors
#define green	"\x1b[32m"
#define yellow	"\x1b[33m"	// Server / Client comm
#define purple	"\x1b[35m"	// Broadcast
#define blue	"\x1b[36m"	// Threads / Proccesses

#define PORT 	5001

static int acc = 5;

void server_stop(int signo)
{
	printf(blue "\n\nYou quit the server\n"
				"Goodbye\n" reset);
	exit(1);
}

void printStatus(int shmid)
{
	sharedMemory *bank = (sharedMemory *)shmat((int)(size_t)shmid, NULL, 0);
	printf("-------------------------------------------------------------\n");

	for(int j = 0; j < 20; j++)
	{
		printf("Account Name: %s\tBalance: %.2f\tIn Session: %d\n", bank->accounts[j].name, bank->accounts[j].balance, bank->accounts[j].inSession);
	}
	printf("\n");
}

void* printBank(void* shmid)
{
	while (1)
	{
		sleep(2);
		printStatus((int)(size_t)shmid);
	}
}

char* doprocessing (char *buffer, int sock, int shmid)
{
	int n;
	bzero(buffer, 256);
	n = read(sock, buffer, 255);

	char sub[5];
	strncpy(sub, buffer, 4);
	sub[4] = '\0';
	// n = write(sock, "hayyyyyylo lol", 20);

	if (n < 0)
	{
		perror(red "ERROR reading from socket" reset);
		exit(1);
	}

	if (strcmp(sub, "exit") == 0)
	{
		printf("GOTTA EXIT \n");
		n = write(sock, "you just typed exit ha", 30);
	}

	if (n < 0)
	{
		perror(red "ERROR writing to socket" reset);
		exit(1);
	}

	// printf("received: %s", buffer);
	// fflush(stdout);

	// printf("size of buffer: %ld\n", strlen(buffer));
	buffer[strlen(buffer)-1] = '\0';
	// printf("size of new buffer: %ld\n", strlen(buffer));
	printf("Received & fixed: %s\n", buffer);

	char* command = strtok(buffer, " ");
	printf("First word of buffer: %s\n", command);

	char* accName = command+strlen(command)+1;
	printf("Rest of buffer: %s\n", accName);


	if (strstr(accName, " ") != NULL)
	{
    	printf(red "ERROR: Account names can not have spaces\n" reset);
	}

	if (strcmp(command, "status") == 0)
	{
		printStatus(shmid);
	}



	sharedMemory *bank = (sharedMemory *)shmat(shmid, NULL, 0);



	/////////////////////////////////////////////////////////////
	//
	//
	//
	//
	//

	if(strcmp(command, "open") == 0)
	{
		// printf("In open\n");

		// Lock array
		if(bank->bankInUse == true)
		{
			printf(red "ERROR: Someone is opening an account, please try again at another time\n" reset);
			// break;
			// exit(1);
		}
		bank->bankInUse = true;

		//check the shared-mem if the account name already exists
		int y = 0;

		for(int x = 0; x < 20; x++) //20 because 20 possible accounts
		{

			if(strcmp(bank->accounts[x].name, accName) == 0)
			{
				printf("The account name '%s' already exists \n", accName);  //Case 1. Account exists
				y = 1;
				bank->bankInUse = false;
				x = 20;
			}


			else if(strcmp(bank->accounts[x].name, "<empty>") == 0)
			{
				strcpy(bank->accounts[x].name, accName);
				printf("The account '%s' is created \n", accName);		//Case 2. Account created
				bank->bankInUse = false;
				y = 1;
				x = 20;

			}
		}

		if(y == 0)
		{
			printf("There are too many accounts, so the account could not be created."); //Case 3. Too many accounts
			bank->bankInUse = false;
		}
	}

	/*
	else if(strcmp(command, "start") == 0)
	{
		int x;
		int y = 0;
		for(x=0; x < 20; x++) //find the account and try to access it
		{
			if(strcmp(bank->accounts[x].name, accName) == 0 && bank->accounts[x].inSession == false) //account exists and is not locked
			{
				printf("The account name '%s' exists at index '%d'\n", accName, x);  //Case 1. Account exists and i got index
				int y = 1;
				bank->accounts[x]->inSession = true;

				\\forever loop to take the other commands while in session
				while(bank->accounts[x]->inSession = true)
				{
					sem_wait(bank->accounts[x].lock);
					n = read(sock, buffer, 255);

					if(strcmp(command, "open") == 0)
					{
						printf("In session, sucks dumbass");
					}
					else if(strcmp(command, "start") == 0)
					{
						printf("In session, stop being dumb and finish ur business");
					}
					else if(strcmp(command, "credit") == 0)
					{
						//add amount
						//write balance
					}
					else if(strcmp(command, "debit") == 0)
					{
						//subtract amount
						//write balance
					}
					else if(strcmp(command, "balance") == 0)
					{
						//write balance
					}
					}
					else if(strcmp(command, "finish") == 0)
					{
						lock = 0;
						bank->accounts[x]->inSession = false;
						sem_post(bank->accounts[x].lock);
						//exit();
					}
					else if(strcmp(command, "exit") == 0)
					{
						lock = 0;
						bank->accounts[x]->inSession = false;
						sem_post(bank->accounts[x].lock);
						//exit(1);
					}else{
						printf("bad command?")
						//exit(1);
					}


					//sub is the string/message we send back.

					n = write(sock, sub,4);

					sem_post(bank->accounts[x].lock);
				}
			}
		}

		if(y = 0)
		{
			printf("The account doesn't exist or is currently being used.");
			exit(1);
		}
		//have to stop the client from opening accounts or logging into other accounts
	} */





////




















	// printf("acc is %d\n", acc);      token = strtok(NULL, s);



	printf("locking sem for account: %s, pid = %d\n", bank->accounts[acc].name, getpid());
	sem_wait(&bank->accounts[acc].lock);

	// printf("Account name %d: %s\n", acc, bank->accounts[acc].name);
	// printf("Account Balance %d: %f\n", acc, bank->accounts[acc].balance);

	bzero(buffer, 256);
	// n = read(sock, buffer, 255);

	printf("unlocking sem for account: %s\n", bank->accounts[acc].name);
	sem_post(&bank->accounts[acc].lock);

	printf("\n\n");
	return buffer;
}



int setupMemory()
{
	// Creating shared memory
	int shmid;
	key_t key;
	sharedMemory *shm;

	int totalSize = sizeof(sharedMemory);

	key = 2215;

	shmid = shmget(key, totalSize, IPC_CREAT | IPC_EXCL | 0666);

	if(shmid < 0)
	{
		shmid = shmget(key, totalSize, 0666);
		shmctl(shmid, IPC_RMID, NULL);
		shmid = shmget(key, totalSize, IPC_CREAT | IPC_EXCL | 0666);
		// printf("%d\n", shmid);
	}

	// Call to create shared memory
	shm = (sharedMemory*)shmat(shmid, NULL, 0);

	memset((sharedMemory *)shm, 0, totalSize);
 	shmdt(shm);

	if(shm == (sharedMemory *) -1)
	{
		perror(red "ERROR with shmat" reset);
		exit(1);
	}

	sharedMemory *bank = (sharedMemory *)shmat(shmid, NULL, 0);

	printf(purple "Initial Bank Accounts: \n");

	for(int j = 0; j < 20; j++)
	{
		char* title = "<empty>";
		strcpy(bank->accounts[j].name, title);

		bank->accounts[j].balance = rand();
		bank->accounts[j].inSession = false;

		sem_t locker;
		bank->accounts[j].lock = locker;
		sem_init(&bank->accounts[j].lock, 1, 1);
	}

	printStatus(shmid);

	return shmid;
}

int main(int argc, char *argv[])
{
	signal(SIGINT, server_stop);

	if (argc != 1)
	{
		printf(red "ERROR: Invalid input\n"
			yellow "Usage: ./server\n" reset);
		exit(1);
	}

	int shmid = setupMemory();







	int sockfd;
	int newsockfd;
	socklen_t clilen;

	// char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	// int n;
	int pid;

	// Call socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		perror(red "ERROR opening socket" reset);
		exit(1);
	}

	// Initialize socket
	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);



	int yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
	    perror("setsockopt");
	    exit(1);
	}






	// Bind address
	while (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror(red "ERROR on binding" reset);
		fprintf(stderr, yellow "Attempting to re-bind to socket in 10 seconds ");

		for (int i=0; i<10; i++)
		{
			usleep(1000000);
			fprintf(stderr, ".");
		}
		printf("\n" reset);
	}








	printf(blue "creating statusThread on server\n" reset);
	pthread_t statusThread;
	if (pthread_create(&statusThread, NULL, printBank, (void*)(size_t)shmid))
	{
		perror(red "ERROR creating thread" reset);
		exit(0);
	}













	printf(blue "Bank Server process (PID = %d) connected and listening on port %d\n", getpid(), PORT);

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	while (1)
	{
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		printf(blue "New client connection opened\n"
					"Client process with pid %d listening\n" reset, getpid());

		if (newsockfd < 0)
		{
			perror(red "ERROR on accept" reset);
			exit(1);
		}

		pid = fork();

		// Forking error
		if (pid < 0)
		{
			perror(red "ERROR on fork" reset);
			exit(1);
		}

		if (pid == 0) // Child process
		{
			char buffer[256];

			while (strcmp(buffer, "exit") != 0)
			{
				strcpy(buffer, doprocessing(buffer, newsockfd, shmid));
			}
			printf(blue "Client connection closed\n"
						"Client process with pid %d exiting\n" reset, getpid());
			close(sockfd);
			exit(0);
		}

		else // Parent process
		{
			close(newsockfd);
		}
	}
}