#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>

#include "utils.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// --- Estructura del cliente ---
typedef struct Client {
    int socket;
    int isAdmin;
    char ip[INET_ADDRSTRLEN];
    int port;
} Client;

// Variables globales
extern Client clients[MAX_CLIENTS];
extern int clientCount;
extern int speed;
extern int battery;
extern int temp;
extern char direction[20];
extern FILE *logFile;

extern pthread_mutex_t clientsMutex;
extern pthread_mutex_t logMutex;

// Funciones
void *handleClient(void *arg);
void *telemetryThread(void *arg);
void logResponse(Client *client, const char *response);

#endif
