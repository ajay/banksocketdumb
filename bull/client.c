#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define reset   "\x1b[0m"
#define red     "\x1b[31m"
#define green   "\x1b[32m"
#define yellow  "\x1b[33m"

void *comm_sender(void* sockfd)
{
	char buffer[6];
	int n;

	// Ask for message
	while (strcmp(buffer, "exit") != 0)
	{
		printf("Please enter one of the following: \n> open \n> start \n> exit\n");
		//bzero(buffer, 256);
		//fgets(buffer, 255, stdin);
		// scanf("%s",buffer);
		char * temp = NULL;
		size_t len;
		// FILE* fp = fdopen((int)(size_t)sockfd, "rw");
		getline(&temp, &len, stdin);

		printf("client temp line: %s\n", temp);

		// while(strcmp(buffer, "open") != 0 && strcmp(buffer, "start") != 0 && strcmp(buffer, "exit") != 0)
		// {
		// 	printf("ERROR! Please enter one of the following 1.open  2.start  3.exit:\n");
		// 	//bzero(buffer, 256);
		// 	//fgets(buffer, 255, stdin);
		// 	scanf("%s",buffer);
		// }

		// Send message to server
		n = write((int)(size_t)sockfd, temp, strlen(temp));

		if (n < 0)
		{
			perror(red "ERROR writing to socket: " reset);
			exit(1);
		}
		sleep(1);
	}
	return NULL;
}

void *comm_listener(void * sockfd)
{
	char buffer[256];
	int n;

	// Read response from server
	bzero(buffer, 256);
	n = read((int)(size_t)sockfd, buffer, 255);

	if (n < 0)
	{
		perror(red "ERROR reading from socket: " reset);
		exit(1);
	}

	printf("Received message: ");
	printf("%s\n", buffer);
	return NULL;
}

int main(int argc, char *argv[])
{
	intptr_t sockfd;
	int portno = 5001;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if (argc != 2)
	{
		printf(red "Why you a dumbass tho\n" reset);
		exit(0);
	}

	// Create socket point
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		perror(red "ERROR opening socket: " reset);
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
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror(red "ERROR connecting: " reset);
		exit(1);
	}

	printf("creating sender\n");
	pthread_t senderThread;
	if (pthread_create(&senderThread, NULL, comm_sender, (void*)sockfd))
	{
		perror(red "ERROR creating thread: " reset);
		exit(0);
	}

	printf("creating listener\n");
	pthread_t listenerThread;
	if (pthread_create(&listenerThread, NULL, comm_listener, (void*)sockfd))
	{
		perror(red "ERROR creating thread: " reset);
		exit(0);
	}

	pthread_join(senderThread, NULL);
	return 0;
}