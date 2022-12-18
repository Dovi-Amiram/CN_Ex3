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
    int dummy = 0;
    
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

    char filedata[fileSize];
    fread(filedata, sizeof(char), fileSize, f);
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
        close(sock);
        return -1;
    }else printf("connected to server\n");

    send(sock, &fileSize, sizeof(int), 0);
    recv(sock, &dummy, sizeof(int), 0);

    char cc_algo[SOCKET_SIZE];
    long bytesSent = -1;
    int xor = 2421 ^ 7494;
    int get_xor;

    while (1)
    {
        strcpy(cc_algo, "cubic");
        socklen_t len = strlen(cc_algo);
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, cc_algo, len) == -1)
        {
            perror("setsockopt");
            return -1;
        }

        printf("Sending part A using cubic CC algorithm\n");
        bytesSent = send(sock, filedata, (fileSize/2), 0);
        if (bytesSent == -1) printf("send() failed with error code : %d", errno);
        else if (bytesSent == 0) printf("peer has closed the TCP connection prior to send().\n");
        else if (bytesSent < (fileSize/2)) printf("sent only %ld bytes from the required %d.\n", bytesSent, (fileSize/2));
        else printf("Sent %d bytes\n", bytesSent);
        
        recv(sock, &get_xor, sizeof(int), 0);
        if (get_xor == xor)
            printf("Part A was successfully sent\n");
        else printf("client: something went wrong\n");

        strcpy(cc_algo, "reno");
        len = strlen(cc_algo);
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, cc_algo, len) != 0) {
            perror("setsockopt");
            return -1;
        }

        printf("Sending part B using reno CC algorithm\n");
        bytesSent = send(sock, filedata+(fileSize/2)-1, (fileSize/2), 0);
        if (bytesSent == -1) printf("send() failed with error code : %d", errno);
        else if (bytesSent == 0) printf("peer has closed the TCP connection prior to send().\n");
        else if (bytesSent < (fileSize/2)) printf("sent only %ld bytes from the required %d.\n", bytesSent, (fileSize/2));
        else printf("Sent %d bytes\n", bytesSent);
        
        printf("Part B was successfully sent\n");
       

        char choose;
        printf("Send again? Y = yes, N = no\n");
        scanf(" %c", &choose);

        recv(sock, &dummy, sizeof(int), 0);

        if (choose == 'N')
        {
            close(sock);
            break;
        }
    }

    return 0;
}

