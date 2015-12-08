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

static int SOCK;
static int acc = 5;

void server_stop(int signo)
{
	if (write(SOCK, "exit", 6) < 0)
	{
		perror(red "ERROR with sending exit to client");
	}
	printf(blue "\n\nYou quit the server\n"
				"Goodbye\n" reset);
	exit(1);
}

void printStatus(int shmid)
{

	sharedMemory *bank = (sharedMemory *)shmat((int)(size_t)shmid, NULL, 0);
	sem_wait(&bank->bankLock);

	printf(purple "-------------------------------------------------------------\n");

	for(int j = 0; j < 20; j++)
	{
		if (strcmp(bank->accounts[j].name, "<empty>") != 0)
			printf("Account Name: %s\tBalance: %.2f\tIn Session: %d\n", bank->accounts[j].name, bank->accounts[j].balance, bank->accounts[j].inSession);
	}
	printf("\n" reset);
	sem_post(&bank->bankLock);

}

void* printBank(void* shmid)
{
	while (1)
	{
		sleep(20);
		printStatus((int)(size_t)shmid);
	}
}

char* doprocessing (char *buffer, int sock, int shmid)
{
	SOCK = sock;
	bzero(buffer, 256);

	if (read(sock, buffer, 255) < 0)
	{
		perror(red "ERROR reading from socket" reset);
		exit(1);
	}

	buffer[strlen(buffer)-1] = '\0';

	printf(yellow "Message received from client: %s\n" reset, buffer);

	char* command = strtok(buffer, " ");
	char* accName = command+strlen(command)+1;

	sharedMemory *bank = (sharedMemory *)shmat(shmid, NULL, 0); //attach shared memory





	if (strcmp(command, "open") == 0)
	{
		bank->bankInUse = true;

		int y = 0;

		for(int x = 0; x < 20; x++)
		{
			if(strcmp(bank->accounts[x].name, accName) == 0)
			{
				char toSend[100];
				sprintf(toSend, "An account with the name '%s' already exists \n", accName);
				if (write(sock, toSend, strlen(toSend)) < 0)
				{
					perror(red "ERROR sending new account message" reset);
				}
				y = 1;
				bank->bankInUse = false;
				x = 20;
			}

			else if(strcmp(bank->accounts[x].name, "<empty>") == 0)
			{
				if(accName[0] == 0x20 || accName[0] == 0x09 || accName[0] == 0x0a || accName[0] == 0x0b || accName[0] == 0x0c || accName[0] == 0x0d)
				{
					strcpy(command,"Can't have account name with spaces!");
					char toSend[100];
					sprintf(toSend, "The accountname with spaces can't be created");
					if (write(sock, toSend, 100) < 0)
					{
						perror(red "ERROR sending account creation message");
					}
					break;
				}
				strcpy(bank->accounts[x].name, accName);
				printf("The account '%s' is created \n", accName);		//Case 2. Account created

				char toSend[100];
				sprintf(toSend, "The account '%s' was created", bank->accounts[x].name);
				if (write(sock, toSend, 100) < 0)
				{
					perror(red "ERROR sending account creation message");
				}

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

	if (strcmp(command, "status") == 0)
	{
		printStatus(shmid);
	}

	if(strcmp(command, "start") == 0)
	{
		int x;
		int y = 0;

		for(x=0; x < 20; x++) //find the account and try to access it
		{
			if(strcmp(bank->accounts[x].name, accName) == 0 && bank->accounts[x].inSession == false) //account exists and is not locked
			{
				printf("The account name '%s' exists at index '%d'\n", accName, x);  //Case 1. Account exists and i got index
				y = 1;
				bank->accounts[x].inSession = true;

				char toSend[100];
				sprintf(toSend, "You just logged into %s", bank->accounts[x].name);
				int poo = write(sock, toSend, strlen(toSend));

				if (poo < 0)
				{
					perror(red "ERROR" reset);
				}

				//forever loop to take the other commands while in session
				while(bank->accounts[x].inSession == true)
				{
					sem_wait(&bank->accounts[x].lock);
					bzero(buffer, 256);
					int n = read(sock, buffer, 255);

					if (n < 0)
					{
						perror(red "ERROR writing to socket" reset);
						exit(1);
					}

					buffer[strlen(buffer)-1] = '\0';

					char* command = strtok(buffer, " ");
					char* amount = command+strlen(command)+1;

					printf("command: %s\n", command);

					char toSend[100];


					if(strcmp(command, "open") == 0)
					{
						sprintf(toSend, "You are already logged into %s\n", bank->accounts[x].name);
					}
					else if(strcmp(command, "start") == 0)
					{

						sprintf(toSend, "You are already loggged into %s\n", bank->accounts[x].name);
					}
					else if(strcmp(command, "credit") == 0)
					{
						float num = atof(amount);
						sprintf(toSend, "Crediting account '%s' by '%.2f'\n", bank->accounts[x].name, num);
						bank->accounts[x].balance += num;

					}
					else if(strcmp(command, "debit") == 0)
					{
						float num = atof(amount);
						if (num < bank->accounts[x].balance)
						{
							sprintf(toSend, "Debiting account '%s' by '%.2f'\n", bank->accounts[x].name, num);
							bank->accounts[x].balance -= num;
						}
						else
						{
							sprintf(toSend, "You can not debit account '%s' by '%.2f'. The balance is too low.\n", bank->accounts[x].name, num);
						}
					}
					else if(strcmp(command, "balance") == 0)
					{
						sprintf(toSend, "The balance of account '%s' is '%.2f'\n", bank->accounts[x].name, bank->accounts[x].balance);
					}
					else if(strcmp(command, "finish") == 0)
					{
						// lock = 0;
						bank->accounts[x].inSession = false;
						sprintf(toSend, "You just logged out of %s", bank->accounts[x].name);
						// x = 20;
						// sem_post(bank->accounts[x].lock);
						//exit();
					}
					else if(strcmp(command, "exit") == 0)
					{
						// lock = 0;
						bank->accounts[x].inSession = false;
						sprintf(toSend, "exit");
						// sem_post(bank->accounts[x].lock);
						//exit(1);
					}
					else
					{
						sprintf(toSend, "Invalid Input");
						//exit(1);
					}

					n = write(sock, toSend, strlen(toSend));

					sem_post(&bank->accounts[x].lock);
				}
				break;
			}
		}

		if(y == 0)
		{
			char toSend[100];
			sprintf(toSend, "Can not start: the account doesn't exist or is currently being used.");
			if (write(sock, toSend, strlen(toSend)) < 0)
			{
				perror(red "ERROR writing start back to client" reset);
			}
		}
		//have to stop the client from opening accounts or logging into other accounts
	}
	else if(strcmp(command, "credit") == 0 || strcmp(command, "debit") == 0 || strcmp(command, "balance") == 0 || strcmp(command, "finish") == 0)
	{
		char send[200];
		sprintf(send,"You are not logged into a account, so %s is an invalid action.\n",command);
		int a;
		a = write(sock, send, strlen(send));

		if(a < 0)
		{
			perror("n<0 ");
		}
	}
	else if(strcmp(command, "exit") == 0)
	{
		char send[200];
		sprintf(send, "exit");
		int a;
		a = write(sock, send, strlen(send));

		if(a < 0)
		{
			perror("n<0 ");
		}
	}



	// printf("acc is %d\n", acc);      token = strtok(NULL, s);

	printf("locking sem for account: %s, pid = %d\n", bank->accounts[acc].name, getpid());
	sem_wait(&bank->accounts[acc].lock);

	// printf("Account name %d: %s\n", acc, bank->accounts[acc].name);
	// printf("Account Balance %d: %f\n", acc, bank->accounts[acc].balance);

	bzero(buffer, 256);
	// n = read(sock, buffer, 255);

	printf("unlocking sem for account: %s\n", bank->accounts[acc].name);
	sem_post(&bank->accounts[acc].lock);

	shmdt(bank); //detach shared memory

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

	for(int j = 0; j < 20; j++)
	{
		char* title = "<empty>";
		strcpy(bank->accounts[j].name, title);

		bank->accounts[j].balance = 0;
		bank->accounts[j].inSession = false;

		sem_t locker;
		bank->accounts[j].lock = locker;
		sem_init(&bank->accounts[j].lock, 1, 1);
	}

	sem_init(&bank->bankLock, 1, 1);

	printf(purple "Initial Bank Accounts: \n" reset);
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

			printf(blue "New client connection opened\n"
				"Client process with pid %d listening\n" reset, getpid());


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