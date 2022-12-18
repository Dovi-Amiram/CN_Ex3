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
    }else printf("executed Bind() successfully\n");

    

    // Make the socket listen.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections = 3
    int listenResult = listen(listeningSocket, 3);
    if (listenResult == -1) {
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }else printf("Waiting for incoming TCP-connections...\n");

    // Accept and incoming connection
    
    struct sockaddr_in clientAddress;  
    memset(&clientAddress, 0, sizeof(clientAddress));
    socklen_t len_clientAddress= sizeof(clientAddress);
   
    int clientSocket = accept(listeningSocket, (struct sockaddr *) &clientAddress, &len_clientAddress);
    if (clientSocket == -1) {
        printf("listen failed with error code : %d", errno);
        // close the sockets
        close(listeningSocket);
        return -1;
    }else printf("A new client connection accepted\n");
    
    char cc_algo[SOCKET_SIZE];
    

    for (int i = 0; i < 2; i++) {

        char part = ' ';
        if (i == 0){
            strcpy(cc_algo, "cubic");
            part = 'A';
        }
        else {
            strcpy(cc_algo, "reno");
            part = 'B';
        }
        socklen_t len = sizeof(cc_algo);
        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, cc_algo, len) != 0) {
            perror("setsockopt");
            return -1;
        }
        if (getsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, cc_algo, &len) != 0) {
            perror("getsockopt");
            return -1;
        }

        int part_size[1];
        recv(clientSocket, part_size, sizeof(int), 0);
        int size = part_size[0] - sizeof(int);
        
        char buffer[BUNDLE];
        long total_byte_count = 0, current_bytes_count = 0;
        //double total_time = 0;

        printf("receiving part %c using %s CC algorithm\n", part, cc_algo);

        struct timeval end, start;
        gettimeofday(&start, NULL);
        while (total_byte_count < (size / BUNDLE * BUNDLE)) {

            current_bytes_count = recv(clientSocket, buffer, BUNDLE, 0);
            total_byte_count += current_bytes_count;

            // if (current_bytes_count == 0) {
            //    printf("Connection with sender closed.\n");
        //}
        }

        if (total_byte_count == size - total_byte_count)
            total_byte_count += recv(clientSocket, buffer, (size - total_byte_count), 0);
        else
            printf("something went wrong!!\n");

        gettimeofday(&end, NULL);

        if (total_byte_count == *part_size) {
            int xor = 2421 ^ 7494;
            write(clientSocket, &xor, sizeof(xor));
        }

        printf("Part %c received. received %ld bytes.\n", part, total_byte_count);

        double time = (double)((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) / 1000000;

        printf("The time it took to receive part %c is %f seconds.\n", part, time);

        total_byte_count = 0;
        buffer[total_byte_count] = '\0';
    }

//
//    printf("\nThe average time it took to get each file (out of 5 samples) in CC method %s is: %f\n\n", CC,
//           avg_time);

printf("Exit\n");
close(listeningSocket);

return 0;
}