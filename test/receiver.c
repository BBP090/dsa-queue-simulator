#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    new_socket = accept(server_fd, NULL, NULL);

    while (1)
    {
        read(new_socket, buffer, 1024);
        printf("Received: %s\n", buffer);
    }

    close(new_socket);
    return 0;
}
