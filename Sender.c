#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "127.0.0.1"
#define FILE_TO_SEND "1mb.txt"
#define SOCKET_SIZE 16

int main() {
    int xor = 2421 ^ 7494;
    char getReply[10];
    int bytesSent = -1;

    /// open the file
    FILE *f = fopen(FILE_TO_SEND, "r");
    if (f == NULL) {
        fprintf(stderr, "Error opening file");
        return 1;
    }

    /// calculate file size
    fseek(f, 0, SEEK_END);
    int fileSize = (int) ftell(f);
    fseek(f, 0, SEEK_SET);

    /// initialize size variables for both buffers
    int size_of_A = fileSize / 2;
    int size_of_B = fileSize - size_of_A;

    /// initialize 2 buffers and allocate memory for both
    char *bufferA, *bufferB;
    bufferA = malloc(sizeof(int) + size_of_A);
    bufferB = malloc(sizeof(int) + size_of_B);

    /// set the size of each of the parts in the start of the array
    memcpy(bufferA, &size_of_A, sizeof(int));
    memcpy(bufferB, &size_of_B, sizeof(int));

    /// read the file splitting it into the 2 buffers
    fread((bufferA) + sizeof(int), 1, size_of_A, f);
    fread((bufferB) + sizeof(int), 1, size_of_B, f);
    /// close the file
    fclose(f);

    /// open a socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        printf("Could not create socket : %d", errno);
        return -1;
    } else printf("New socket opened\n");


    /// "sockaddr_in" is the "derived" from sockaddr structure
    /// used for IPv4 communication. For IPv6, use sockaddr_in6
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    // little endian => network endian (big endian):
    serverAddress.sin_port = htons(SERVER_PORT);
    /// convert IPv4 and IPv6 addresses from text to binary form
    int rval = inet_pton(AF_INET, (const char *) SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    // e.g. 127.0.0.1 => 0x7f000001 => 01111111.00000000.00000000.00000001 => 2130706433
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    }

    // Make a connection to the server with socket SendingSocket.
    int connectResult = connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (connectResult == -1) {
        printf("connect() failed with error code : %d", errno);
        // cleanup the socket;
        close(sock);
        return -1;
    }

    // get ACK
    bzero(getReply, sizeof(getReply));
    read(sock, getReply, sizeof(getReply));
    if (strcmp(getReply, "ACK") == 0) printf("From Server : %s\n", getReply);
    else printf("The Server didnt answer\n");

    printf("connected to server\n");

    // char array to save CC algo name
    char cc_algo[SOCKET_SIZE];

    // initialize as 'Y' to allow first time sending
    char userDecision = 'Y';

    while (userDecision != 'N') {
        for (int j = 0; j < 2; j++) {
            if (j == 0) strcpy(cc_algo, "cubic");
            else strcpy(cc_algo, "reno");

            socklen_t len;

            len = sizeof(cc_algo);
            if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, cc_algo, len) != 0) {
                perror("setsockopt");
                return -1;
            }

            if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, cc_algo, &len) != 0) {
                perror("getsockopt");
                return -1;
            }

            if (j == 0) {
                printf("Sending part A using cubic CC algorithm\n");
                bytesSent = send(sock, bufferA, size_of_A, 0);
                if (bytesSent == -1) {
                    printf("send() failed with error code : %d", errno);
                } else if (bytesSent == 0) {
                    printf("peer has closed the TCP connection prior to send().\n");
                } else if (bytesSent < size_of_A) {
                    printf("sent only %d bytes from the required %d.\n", bytesSent, size_of_A);
                } else {
                    bzero(getReply, sizeof(getReply));
                    read(sock, getReply, sizeof(getReply));
                    if (getReply == 1234 ^ 5678)
                        printf("Part A was successfully sent\n", getReply);
                    else printf("client: something went wrong\n");
                }
            } else {
                printf("Sending part B using reno CC algorithm\n");
                bytesSent = send(sock, bufferB, size_of_B, 0);
                if (bytesSent == -1) {
                    printf("send() failed with error code : %d", errno);
                } else if (bytesSent == 0) {
                    printf("Receiver doesn't accept requests!.\n");
                } else if (bytesSent < size_of_B) {
                    printf("sent only %d bytes from the required %d.\n", bytesSent, size_of_B);
                } else {
                    printf("Sent total %d bytes\n", bytesSent);
                }
            }

            bzero(getReply, sizeof(getReply));
            read(sock, getReply, sizeof(getReply));
            if (strcmp(getReply, "ACK") == 0)
                printf("From Server : %s\n", getReply);
            else printf("The Server didnt answer\n");
        }


        printf("Would you like to send the file again? (Y/N)");
        scanf("%c", &userDecision);
    }

    close(sock);
    return 0;
}

