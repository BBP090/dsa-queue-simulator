#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    while (1)
    {
        snprintf(buffer, sizeof(buffer), "Vehicle%d", rand() % 100);
        send(sockfd, buffer, strlen(buffer), 0);
        printf("Sent: %s\n", buffer);
        sleep(1);
    }

    close(sockfd);
    return 0;
}
