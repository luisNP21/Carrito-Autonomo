#include "client_handler.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <puerto> <archivo_log>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);
    logFile = fopen(argv[2], "a");
    if (!logFile) {
        perror("Error abriendo archivo de log");
        return -1;
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error creando socket");
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Error en listen");
        return -1;
    }

    printf("Servidor escuchando en puerto %d\n", port);
    fprintf(logFile, "Servidor escuchando en puerto %d\n", port);

    pthread_t telemetry_tid;
    pthread_create(&telemetry_tid, NULL, telemetryThread, NULL);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (new_socket < 0) {
            perror("Error en accept");
            continue;
        }

        pthread_mutex_lock(&clientsMutex);
        if (clientCount < MAX_CLIENTS) {
            clients[clientCount].socket = new_socket;
            clients[clientCount].isAdmin = 0;
            inet_ntop(AF_INET, &address.sin_addr,
                      clients[clientCount].ip, INET_ADDRSTRLEN);
            clients[clientCount].port = ntohs(address.sin_port);

            pthread_t tid;
            pthread_create(&tid, NULL, handleClient, &clients[clientCount]);
            pthread_detach(tid);

            clientCount++;
        } else {
            sendMessage(new_socket, "ERROR|SERVER FULL\n");
            close(new_socket);
        }
        pthread_mutex_unlock(&clientsMutex);
    }

    fclose(logFile);
    close(server_fd);
    return 0;
}
