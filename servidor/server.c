#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <puerto> <archivo_log>\n", argv[0]);
        return -1;
    }

    //tcp.port == 8080 filtro de wireshark para ver el trafico
    //tcp.port == 8080 && tcp.len > 0 filtro de mensaje en wireshark

    int port = atoi(argv[1]);
    FILE *logFile = fopen(argv[2], "a");

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    fprintf(logFile, "Servidor escuchando en puerto %d\n", port);
    fflush(logFile);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        char *msg = "Conectado al servidor\n";
        send(new_socket, msg, strlen(msg), 0);
        fprintf(logFile, "Cliente conectado desde %s:%d\n",
            inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        fflush(logFile);
        close(new_socket);
    }

    fclose(logFile);
    close(server_fd);
    return 0;
}
