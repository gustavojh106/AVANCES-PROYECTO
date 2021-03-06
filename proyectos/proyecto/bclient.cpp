// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT	 8080 
#define MAXLINE 1024 

// Driver code 
int main() { 
	int sockfd; 
	char buffer[MAXLINE]; 
	int broadcast = 1;
	char *hello = "Hello from client"; 
	struct sockaddr_in	 servaddr; 

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	setsockopt( sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof( broadcast ) );

	memset(&servaddr, 0, sizeof(servaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(PORT); 
	//servaddr.sin_addr.s_addr = INADDR_ANY; 
	char * baddr = "172.16.123.31";
	if (inet_aton(baddr, &(servaddr.sin_addr)) == 0)
    {
        printf("sendto: could not convert address: %s\n", baddr);
        return 0;
    }

	socklen_t len; 
	int n;
	
	sendto(sockfd, (const char *)hello, strlen(hello), 
		MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
			sizeof(servaddr)); 
	printf("Hello message sent.\n"); 
		
	n = recvfrom(sockfd, (char *)buffer, MAXLINE, 
				MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
	buffer[n] = '\0'; 
	printf("Server : %s\n", buffer); 

	close(sockfd); 
	return 0; 
} 
