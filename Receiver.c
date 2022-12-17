#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#define SERVER_PORT 5060
#define BUFFER_SIZE 1024
#define SOCKET_SIZE 16
#define BUNDLE 32768

int main() {
    socklen_t len;
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
        // close the socket
        close(listeningSocket);
        return -1;
    }

    printf("executed Bind() successfully\n");

    // Make the socket listen.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections = 3
    int listenResult = listen(listeningSocket, 3);
    if (listenResult == -1) {
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    // Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");
    struct sockaddr_in clientAddress;  //
    socklen_t clientAddressLen = sizeof(clientAddress);
    char sendbuffer[BUFFER_SIZE] = {'\0'};


    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddressLen = sizeof(clientAddress);
    int clientSocket = accept(listeningSocket, (struct sockaddr *) &clientAddress, &clientAddressLen);
    if (clientSocket == -1) {
        printf("listen failed with error code : %d", errno);
        // close the sockets
        close(listeningSocket);
        return -1;
    }


    for (int i = 0; i < 2; i++) {

        printf("A new client connection accepted\n");

        int *part_size;
        recv(clientSocket, part_size, sizeof(int), 0);
        printf("%d", *part_size);


        char cc_algo[SOCKET_SIZE];

        if (i == 0)
            strcpy(cc_algo, "cubic");
        else strcpy(cc_algo, "reno");

        if (getsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, cc_algo, &len) != 0) {
            perror("getsockopt");
            return -1;
        }

        if (i == 0)
            printf("receiving part A using cubic CC algorithm\n");
        else
            printf("receiving part B using reno CC algorithm\n");

        char buffer[BUNDLE];

        long total_byte_count = 0, current_bytes_count = 0;
        double total_time = 0;

        struct timeval end, start;
        gettimeofday(&start, NULL);

        while (total_byte_count < ((*part_size) / BUNDLE) * BUNDLE) {
            current_bytes_count = recv(clientSocket, buffer, BUNDLE, 0);
            total_byte_count += current_bytes_count;

            if (current_bytes_count == 0) {
                printf("Connection with sender closed.\n");
            }
        }
        total_byte_count += recv(clientSocket, buffer, (*part_size - total_byte_count), 0);

        gettimeofday(&end, NULL);
        printf("Part A received. received %ld bytes.\n", total_byte_count);

        double time =
                (double) ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) / 1000000;

        printf("The time it took to receive part A is %f seconds.\n", time);

        total_byte_count = 0;
        buffer[total_byte_count] = '\0';
    }

  //      double avg_time = total_time / 5;
  //      char CC[100];
  //      if (i == 0)
            strcpy(CC, "cubic");
        else

            strcpy(CC, "reno");

        printf("\nThe average time it took to get each file (out of 5 samples) in CC method %s is: %f\n\n", CC,
               avg_time);
    }

    printf("Exit\n");
    close(listeningSocket);

    return 0;
}