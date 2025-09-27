#include "client_handler.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Client clients[MAX_CLIENTS];
int clientCount = 0;
int speed = 120;
int battery = 100;
int temp = 25;
char direction[20] = "N/A";
FILE *logFile;

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

// --- Log de respuestas ---
void logResponse(Client *client, const char *response) {
    pthread_mutex_lock(&logMutex);
    fprintf(logFile, "Respuesta a %s:%d [%s] -> %s\n",
            client->ip,
            client->port,
            client->isAdmin ? "Admin" : "Observador",
            response);
    fflush(logFile);
    pthread_mutex_unlock(&logMutex);
}

// --- Enviar mensajes con log ---


// --- Telemetría ---
void broadcastTelemetry() {
    char telemetry[BUFFER_SIZE];
    snprintf(telemetry, sizeof(telemetry),
             "TELEMETRY|speed=%d;bat=%d;temp=%d;dir=%s\n",
             speed, battery, temp, direction);

    pthread_mutex_lock(&clientsMutex);
    for (int i = 0; i < clientCount; i++) {
        sendMessageToClient(&clients[i], telemetry);
    }
    pthread_mutex_unlock(&clientsMutex);
}

void *telemetryThread(void *arg) {
    while (1) {
        sleep(1);
        broadcastTelemetry();
    }
    return NULL;
}

// --- Manejo de cliente ---
void *handleClient(void *arg) {
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];

    printf("Cliente conectado: %s:%d\n", client->ip, client->port);

    pthread_mutex_lock(&logMutex);
    fprintf(logFile, "Cliente conectado: %s:%d\n", client->ip, client->port);
    fflush(logFile);
    pthread_mutex_unlock(&logMutex);

    sendMessageToClient(client, "ACK|CONNECTED\n");

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
        buffer[strcspn(buffer, "\r\n")] = 0; // limpiar saltos de línea

        printf("Mensaje recibido de %s:%d [%s] -> %s\n",
               client->ip, client->port,
               client->isAdmin ? "Admin" : "Observador", buffer);

        pthread_mutex_lock(&logMutex);
        fprintf(logFile, "Mensaje de %s:%d [%s] -> %s\n",
                client->ip, client->port,
                client->isAdmin ? "Admin" : "Observador", buffer);
        fflush(logFile);
        pthread_mutex_unlock(&logMutex);

        // --- Protocolo ---
        if (strncmp(buffer, "LOGIN|", 6) == 0) {
            if (strcmp(buffer + 6, "admin:1234") == 0) {
                client->isAdmin = 1;
                sendMessageToClient(client, "ACK|LOGIN SUCCESS\n");
            } else {
                sendMessageToClient(client, "ERROR|INVALID CREDENTIALS\n");
            }
        }
        else if (strncmp(buffer, "LOGOUT|", 7) == 0) {
            sendMessageToClient(client, "ACK|LOGOUT SUCCESS\n");

            printf("Cliente %s:%d cerró sesión con LOGOUT.\n",
                   client->ip, client->port);

            pthread_mutex_lock(&logMutex);
            fprintf(logFile, "Cliente %s:%d cerró sesión con LOGOUT.\n",
                    client->ip, client->port);
            fflush(logFile);
            pthread_mutex_unlock(&logMutex);

            close(client->socket);

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
                sendMessageToClient(client, usersList);
            } else {
                sendMessageToClient(client, "ERROR|NOT AUTHORIZED\n");
            }
        }
        else if (strncmp(buffer, "COMMAND|", 8) == 0) {
            if (client->isAdmin) {
                char *cmd = buffer + 8;

                if (strcmp(cmd, "SPEED UP") == 0) {
                    if (battery <= 10)
                        sendMessageToClient(client, "ERROR|LOW BATTERY\n");
                    else if (speed >= 120)
                        sendMessageToClient(client, "ERROR|SPEED LIMIT REACHED\n");
                    else {
                        speed += 5; battery -= 2;
                        strcpy(direction, "Acelerando");
                        sendMessageToClient(client, "ACK|SPEED UP\n");
                    }
                }
                else if (strcmp(cmd, "SLOW DOWN") == 0) {
                    if (speed > 0) {
                        speed -= 5; if (speed < 0) speed = 0;
                        battery -= 1;
                        strcpy(direction, "Frenando");
                        sendMessageToClient(client, "ACK|SLOW DOWN\n");
                    } else sendMessageToClient(client, "ERROR|MIN SPEED\n");
                }
                else if (strcmp(cmd, "TURN LEFT") == 0) {
                    if (battery <= 5)
                        sendMessageToClient(client, "ERROR|LOW BATTERY\n");
                    else {
                        battery -= 1;
                        strcpy(direction, "Izquierda");
                        sendMessageToClient(client, "ACK|TURN LEFT\n");
                    }
                }
                else if (strcmp(cmd, "TURN RIGHT") == 0) {
                    if (battery <= 5)
                        sendMessageToClient(client, "ERROR|LOW BATTERY\n");
                    else {
                        battery -= 1;
                        strcpy(direction, "Derecha");
                        sendMessageToClient(client, "ACK|TURN RIGHT\n");
                    }
                }
                else sendMessageToClient(client, "ERROR|UNKNOWN COMMAND\n");

                if (battery <= 0) {
                    sendMessageToClient(client, "ERROR|BATTERY EMPTY\n");
                    close(client->socket);
                    break;
                }
            } else sendMessageToClient(client, "ERROR|NOT AUTHORIZED\n");
        }
        else {
            sendMessageToClient(client, "ERROR|UNKNOWN MESSAGE\n");
        }
    }
    return NULL;
}
