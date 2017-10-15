//
// Created by MightyPork on 2017/10/15.
//

// http://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner

#include "demo.h"

#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>

volatile int sockfd = -1;
volatile bool conn_disband = false;

void demo_disconn(void)
{
	conn_disband = true;
	if (sockfd >= 0) close(sockfd);
}

void TF_WriteImpl(const uint8_t *buff, size_t len)
{
	printf("\033[32m--- TX %ld bytes ---\033[0m\n", len);
	dumpFrame(buff, len);
	usleep(1000);

	if (sockfd != -1) {
		write(sockfd, buff, len);
	} else {
		printf("\nNo peer!\n");
	}
}

static int demo_client(void* unused)
{
	(void)unused;

	ssize_t n = 0;
	uint8_t recvBuff[1024];
	struct sockaddr_in serv_addr;

	printf("\n--- STARTING CLIENT! ---\n");

	memset(recvBuff, '0', sizeof(recvBuff));
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Error : Could not create socket \n");
		return false;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\n inet_pton error occured\n");
		return false;
	}

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("\n Error : Connect Failed \n");
		perror("PERROR ");
		return false;
	}

	printf("\n Child Process \n");

	while ((n = read(sockfd, recvBuff, sizeof(recvBuff) - 1)) > 0) {
		printf("\033[36m--- RX %ld bytes ---\033[0m\n", n);
		dumpFrame(recvBuff, (size_t) n);
		TF_Accept(recvBuff, (size_t) n);
	}
	return 0;
}

static int demo_server(void* unused)
{
	(void)unused;
	ssize_t n;
	int listenfd = 0;
	uint8_t recvBuff[1024];
	struct sockaddr_in serv_addr;
	int option;

	printf("\n--- STARTING SERVER! ---\n");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	option = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Failed to bind");
		return 1;
	}

	if (listen(listenfd, 10) < 0) {
		perror("Failed to listen");
		return 1;
	}

	while (1) {
		printf("\nWaiting for client...\n");
		sockfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));
		printf("\nClient connected\n");
		conn_disband = false;

		while ((n = read(sockfd, recvBuff, sizeof(recvBuff) - 1)) > 0 && !conn_disband) {
			printf("\033[36m--- RX %ld bytes ---\033[0m\n", n);
			dumpFrame(recvBuff, n);
			TF_Accept(recvBuff, (size_t) n);
		}

		if (n < 0) {
			printf("\n Read error \n");
		}

		printf("Closing socket\n");
		close(sockfd);
		sockfd = -1;
	}
	return 0;
}

void signal_handler(int sig)
{
	(void)sig;
	printf("Shutting down...");
	demo_disconn();
	exit(sig);
}

void demo_sleep(void)
{
	while(1) usleep(10);
}

void demo_init(TF_PEER peer)
{
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	int retc;
	void *stack = malloc(8192);
	if (stack == NULL) {
		perror("Oh fuck");
		signal_handler(9);
		return;
	}

	printf("Starting %s...\n", peer == TF_MASTER ? "MASTER" : "SLAVE");
	if (peer == TF_MASTER) {
		retc = clone(&demo_client, (char *)stack+8192, CLONE_VM|CLONE_FILES, 0);
	} else {
		retc = clone(&demo_server, (char *)stack+8192, CLONE_VM|CLONE_FILES, 0);
	}

	if (retc == 0) {
		perror("Clone fail");
		signal_handler(9);
		return;
	}

	printf("Thread started\n");
}
