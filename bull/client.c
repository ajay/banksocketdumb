#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define reset	"\x1b[0m"
#define red 	"\x1b[31m"	// Errors
#define green	"\x1b[32m"	// Messages from server
#define yellow	"\x1b[33m"	// Server / Client comm
#define blue	"\x1b[36m"	// Threads / Proccesses

void client_stop(int signo)
{
	printf(blue "\n\nYou quit the client\n"
				"Goodbye\n" reset);
	exit(1);
}

void *comm_sender(void* sockfd)
{
	char* buffer = "";
	while (strcmp(buffer, "exit") != 0)
	{
		// Ask for message
		printf("\n----------------------------------\n");
		printf("Please enter one of the following: \n"
				"> open <accountname> \n"
				"> start <accountname> \n"
				"> credit <amount> \n"
				"> debit <amount> \n"
				"> balance \n"
				"> finish \n"
				"> exit\n\n"
				"> ");

		buffer = NULL;
		size_t len;

		if (getline(&buffer, &len, stdin) < 0)
		{
			perror(red "ERROR with getline" reset);
		};

		// Send message to server
		if (write((int)(size_t)sockfd, buffer, strlen(buffer)) < 0)
		{
			perror(red "ERROR writing to socket" reset);
			exit(1);
		}
		sleep(1);
	}
	return NULL;
}

void *comm_listener(void* sockfd)
{
	while (1)
	{
		char buffer[256];
		bzero(buffer, 256);

		if (read((int)(size_t)sockfd, buffer, 255) < 0)
		{
			perror(red "ERROR reading from socket" reset);
		}

		printf(green "Received message from server: " reset);
		printf("%s\n", buffer);
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	signal(SIGINT, client_stop);
	intptr_t sockfd;
	int portno = 5001;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if (argc != 2)
	{
		printf(red "ERROR: Invalid input\n"
			yellow "Usage: ./client <server_ip_address>\n" reset);
		exit(0);
	}

	// Create socket point
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		perror(red "ERROR opening socket" reset);
		exit(1);
	}

	server = gethostbyname(argv[1]);

	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	// Connect to server
	while (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror(red "ERROR connecting" reset);
		fprintf(stderr, yellow "Attempting to re-connect to server in 3 seconds ");

		for (int i=0; i<10; i++)
		{
			usleep(300000);
			fprintf(stderr, ".");
		}
		printf("\n" reset);
	}

	printf(blue "creating sender thread for client\n" reset);
	pthread_t senderThread;
	if (pthread_create(&senderThread, NULL, comm_sender, (void*)sockfd))
	{
		perror(red "ERROR creating thread" reset);
		exit(0);
	}

	printf(blue "creating listener thread for client\n" reset);
	pthread_t listenerThread;
	if (pthread_create(&listenerThread, NULL, comm_listener, (void*)sockfd))
	{
		perror(red "ERROR creating thread" reset);
		exit(0);
	}

	pthread_join(senderThread, NULL);
	// pthread_join(listenerThread, NULL);

	return 0;
}