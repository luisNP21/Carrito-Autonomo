#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Estructura para clientes
typedef struct {
    int socket;
    int isAdmin;
    char ip[INET_ADDRSTRLEN];
    int port;
} Client;

Client clients[MAX_CLIENTS];
int clientCount = 0;
int speed = 120;      // velocidad inicial
int battery = 100;    // batería inicial
int temp = 25;        // temperatura inicial
char direction[20] = "N/A"; // Dirección inicial
FILE *logFile;

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

// --- Funciones auxiliares ---
void sendMessage(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

void broadcastTelemetry() {
    char telemetry[BUFFER_SIZE];

    snprintf(telemetry, sizeof(telemetry),
             "TELEMETRY|speed=%d;bat=%d;temp=%d;dir=%s\n",
             speed, battery, temp, direction);

    pthread_mutex_lock(&clientsMutex);
    for (int i = 0; i < clientCount; i++) {
        sendMessage(clients[i].socket, telemetry);
    }
    pthread_mutex_unlock(&clientsMutex);
}

void *telemetryThread(void *arg) {
    while (1) {
        sleep(1);  // cada 1 segundo
        broadcastTelemetry();
    }
    return NULL;
}

// --- Manejo de cliente ---
void *handleClient(void *arg) {
    Client *client = (Client *)arg;   // Usar puntero, no copia
    char buffer[BUFFER_SIZE];

    printf("Cliente conectado: %s:%d\n", client->ip, client->port);

    // Log conexión
    pthread_mutex_lock(&logMutex);
    fprintf(logFile, "Cliente conectado: %s:%d\n", client->ip, client->port);
    fflush(logFile);
    pthread_mutex_unlock(&logMutex);

    // Mensaje de bienvenida
    sendMessage(client->socket, "ACK|CONNECTED\n");

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client->socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0) {
            printf("Cliente desconectado: %s:%d\n", client->ip, client->port);

            pthread_mutex_lock(&logMutex);
            fprintf(logFile, "Cliente desconectado: %s:%d\n", client->ip, client->port);
            fflush(logFile);
            pthread_mutex_unlock(&logMutex);

            close(client->socket);

            // Eliminar cliente del array global
            pthread_mutex_lock(&clientsMutex);
            for (int i = 0; i < clientCount; i++) {
                if (clients[i].socket == client->socket) {
                    clients[i] = clients[clientCount - 1];
                    clientCount--;
                    break;
                }
            }
            pthread_mutex_unlock(&clientsMutex);

            break;
        }

        buffer[bytes] = '\0';
        printf("Mensaje recibido de %s:%d [%s] -> %s\n",
               client->ip,
               client->port,
               client->isAdmin ? "Admin" : "Observador",
               buffer);

        pthread_mutex_lock(&logMutex);
        fprintf(logFile, "Mensaje de %s:%d [%s] -> %s\n",
                client->ip,
                client->port,
                client->isAdmin ? "Admin" : "Observador",
                buffer);
        fflush(logFile);
        pthread_mutex_unlock(&logMutex);

        // --- Protocolo ---
        if (strncmp(buffer, "LOGIN|", 6) == 0) {
            if (strcmp(buffer + 6, "admin:1234") == 0) {
                client->isAdmin = 1;
                sendMessage(client->socket, "ACK|LOGIN SUCCESS\n");
            } else {
                sendMessage(client->socket, "ERROR|INVALID CREDENTIALS\n");
            }
        }
        else if (strncmp(buffer, "REQUEST|USERS", 13) == 0) {
            if (client->isAdmin) {
                char usersList[BUFFER_SIZE];
                memset(usersList, 0, BUFFER_SIZE);
                strcpy(usersList, "USERS|");

                pthread_mutex_lock(&clientsMutex);
                for (int i = 0; i < clientCount; i++) {
                    char userInfo[64];
                    snprintf(userInfo, sizeof(userInfo), "%s:%d%s",
                             clients[i].ip,
                             clients[i].port,
                             (i < clientCount - 1) ? ";" : "");
                    strcat(usersList, userInfo);
                }
                pthread_mutex_unlock(&clientsMutex);

                strcat(usersList, "\n");
                sendMessage(client->socket, usersList);

                printf("Admin %s:%d solicitó lista de usuarios.\n",
                       client->ip, client->port);
            } else {
                sendMessage(client->socket, "ERROR|NOT AUTHORIZED\n");
                printf("Cliente %s:%d intentó solicitar usuarios sin permisos.\n",
                       client->ip, client->port);
            }
        }
        else if (strncmp(buffer, "COMMAND|", 8) == 0) {
            if (client->isAdmin) {
                char *cmd = buffer + 8;

                if (strcmp(cmd, "SPEED UP") == 0) {
                    if (battery <= 10) {
                        sendMessage(client->socket, "ERROR|LOW BATTERY\n");
                    } else if (speed >= 120) {
                        sendMessage(client->socket, "ERROR|SPEED LIMIT REACHED\n");
                    } else {
                        speed += 5;
                        battery -= 2;
                        strcpy(direction, "Acelerando");
                        sendMessage(client->socket, "ACK|SPEED UP\n");
                    }
                }
                else if (strcmp(cmd, "SLOW DOWN") == 0) {
                    if (speed > 0) {
                        speed -= 5;
                        if (speed < 0) speed = 0;
                        battery -= 1;
                        strcpy(direction, "Frenando");
                        sendMessage(client->socket, "ACK|SLOW DOWN\n");
                    } else {
                        sendMessage(client->socket, "ERROR|MIN SPEED\n");
                    }
                }
                else if (strcmp(cmd, "TURN LEFT") == 0) {
                    if (battery <= 5) {
                        sendMessage(client->socket, "ERROR|LOW BATTERY\n");
                    } else {
                        battery -= 1;
                        strcpy(direction, "Izquierda");
                        sendMessage(client->socket, "ACK|TURN LEFT\n");
                    }
                }
                else if (strcmp(cmd, "TURN RIGHT") == 0) {
                    if (battery <= 5) {
                        sendMessage(client->socket, "ERROR|LOW BATTERY\n");
                    } else {
                        battery -= 1;
                        strcpy(direction, "Derecha");
                        sendMessage(client->socket, "ACK|TURN RIGHT\n");
                    }
                }
                else {
                    sendMessage(client->socket, "ERROR|UNKNOWN COMMAND\n");
                }

                if (battery <= 0) {
                    sendMessage(client->socket, "ERROR|BATTERY EMPTY\n");
                    printf("Cliente %s:%d quedó sin batería. Cerrando conexión.\n",
                           client->ip, client->port);
                    close(client->socket);
                    break;
                }
            } else {
                printf("Cliente %s:%d intentó enviar comando sin permisos.\n",
                       client->ip, client->port);
                sendMessage(client->socket, "ERROR|NOT AUTHORIZED\n");
            }
        }
        else {
            sendMessage(client->socket, "ERROR|UNKNOWN MESSAGE\n");
        }
    }

    return NULL;
}

// --- Main ---
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

    srand(time(NULL));

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
            inet_ntop(AF_INET, &address.sin_addr, clients[clientCount].ip, INET_ADDRSTRLEN);
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
