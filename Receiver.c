#include <stdio.h>
//#include <stdlib.h>
//#include <sys/types.h>
#include <sys/socket.h>
//#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
//#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#define SERVER_PORT 5060
#define SOCKET_SIZE 16
#define BUNDLE 32768

int main() {
    int dummy = 0;
    // signal(SIGPIPE, SIG_IGN);  // on linux to prevent crash on closing socket

    // Open the listening (server) socket

    int listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // 0 means default protocol for stream sockets (Equivalently, IPPROTO_TCP)
    if (listeningSocket == -1) {
        printf("Could not create listening socket : %d", errno);
        return 1;
    }

    // Reuse the address if the server socket was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    int enableReuse = 1;
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        printf("setsockopt() failed with error code : %d", errno);
        return 1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    //
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY; // any IP at this port (Address to accept any incoming messages)
    serverAddress.sin_port = htons(SERVER_PORT);  // network order (makes byte order consistent)

    // Bind the socket to the port with any IP at this port
    int bindResult = bind(listeningSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (bindResult == -1) {
        printf("Bind failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    }else printf("executed Bind() successfully\n");

    // Make the socket listen.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections = 3
    int listenResult = listen(listeningSocket, 3);
    if (listenResult == -1) {
        printf("listen() failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    }else printf("Waiting for incoming TCP-connections...\n");

    // Accept and incoming connection
    
    struct sockaddr_in clientAddress;  
    memset(&clientAddress, 0, sizeof(clientAddress));
    socklen_t len_clientAddress = sizeof(clientAddress);
   
    int clientSocket = accept(listeningSocket, (struct sockaddr *) &clientAddress, &len_clientAddress);
    if (clientSocket == -1) {
        printf("listen failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    }else printf("A new client connection accepted\n");

    int fileSize;
    recv(clientSocket, &fileSize, sizeof(int), 0);
    send(clientSocket, &dummy, sizeof(int), 0);
    
    char cc_algo[SOCKET_SIZE];
    int running = 1;
    int xor = 2421 ^ 7494;

    while(running)
    {
        char buffer[fileSize/2];
        int totalbytes = 0;

        printf("Changing to cubic...\n");
        strcpy(cc_algo, "cubic");
        socklen_t len = strlen(cc_algo);
        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, cc_algo, len) == -1) {
            perror("setsockopt");
            return -1;
        }

        printf("Waiting for part A...\n");

        while (totalbytes < (fileSize/2))
        {
            int bytesgot = recv(clientSocket, buffer+totalbytes, sizeof(char), 0);
            totalbytes += bytesgot;

            if (bytesgot == 0)
            {
                printf("Connection with sender closed, exiting...\n");
                running = 0;
                break;
            }
        }

        if (running == 0)
            break;

        printf("Got part A\n");

        printf("Sending authntication check\n");
        send(clientSocket, &xor, sizeof(int), 0);
        printf("Authontication sent\n");

        printf("Changeing to reno..\n");

        strcpy(cc_algo, "reno");
        len = strlen(cc_algo);
        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, cc_algo, len) == -1) {
            perror("setsockopt");
            return -1;
        }

        totalbytes = 0;

        printf("Waiting for part B...\n");

        while (totalbytes < (fileSize/2))
        {
            int bytesgot = recv(clientSocket, buffer+totalbytes, sizeof(char), 0);
            totalbytes += bytesgot;

            if (bytesgot == 0)
            {
                printf("Connection with sender closed, exiting...\n");
                running = 0;
                break;
            }
        }

        if (running == 0)
            break;

        printf("Got part B\n");
        send(clientSocket, &dummy, sizeof(int), 0);
    }

    printf("Exit\n");
    close(clientSocket);
    close(listeningSocket);

    return 0;
}