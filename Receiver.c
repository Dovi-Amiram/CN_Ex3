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
#include <time.h>
#include <unistd.h>

#define SERVER_PORT 5060
#define BUFFER_SIZE 1024
#define SOCKET_SIZE 16
 
int main() {
    socklen_t len;
    // signal(SIGPIPE, SIG_IGN);  // on linux to prevent crash on closing socket

    // Open the listening (server) socket
    int listeningSocket = -1;
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 0 means default protocol for stream sockets (Equivalently, IPPROTO_TCP)
    if (listeningSocket == -1) {
        printf("Could not create listening socket : %d", errno);
        return 1;
    }

    // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    //
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
    int bindResult = bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (bindResult == -1) {
        printf("Bind failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    printf("Bind() success\n");

    // Make the socket listening; actually mother of all client sockets.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections
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

    for (int i = 0; i < 2; i++) {
        int total_byte_count = 0;
        int sending_attempt = 1;
        double total_time = 0;
        while (sending_attempt <= 5) {
            memset(&clientAddress, 0, sizeof(clientAddress));
            clientAddressLen = sizeof(clientAddress);
            int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
            if (clientSocket == -1) {
                printf("listen failed with error code : %d", errno);
                // close the sockets
                close(listeningSocket);
                return -1;
            }

            printf("A new client connection accepted\n");

            int current_bytes_count;
            char reply[10] = "ACK";
            write(clientSocket, reply, sizeof(reply));
            char buf[100];

            struct timeval end, start;
            gettimeofday(&start, NULL);
             do {
                current_bytes_count = recv(clientSocket, buf, 100, 0);
                total_byte_count += current_bytes_count;

                if (current_bytes_count == 0)
                {
                    printf("Connection with sender closed.\n");
                }
                printf("First part received, bytes: %d.\n", total_byte_count);
            } while (current_bytes_count < (BUFFER_SIZE/2));


            gettimeofday(&end, NULL);

            double current_attempt_time = (double) ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) / 1000000;
            total_time += current_attempt_time;
            printf("The time it took to receive the file from the client was %f seconds.\n", current_attempt_time);

            total_byte_count = 0;
            printf("Listening...\n");
            buf[current_bytes_count] = '\0';

            sleep(1);
        }

        double avg_time = total_time / 5;
        char CC[100];
        if (i == 0)
            strcpy(CC, "cubic");
        else
            strcpy(CC, "reno");

        printf("\nThe average time it took to get each file (out of 5 samples) in CC method %s is: %f\n\n", CC, avg_time);

    }

    printf("Exit\n");
    close(listeningSocket);

    return 0;
}