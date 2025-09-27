#include "utils.h"
#include <stdio.h>
#include <string.h>
#include "client_handler.h"
#include <unistd.h>

// --- Enviar mensaje simple (sin log) ---
void sendMessage(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

// --- Enviar mensaje con log ---
void sendMessageToClient(Client *client, const char *msg) {
    extern FILE *logFile;
    extern pthread_mutex_t logMutex;

    // Enviar siempre
    send(client->socket, msg, strlen(msg), 0);

    // Solo guardar en log si NO es telemetría
    if (strncmp(msg, "TELEMETRY|", 10) != 0) {
        pthread_mutex_lock(&logMutex);
        fprintf(logFile, "Respuesta a %s:%d [%s] -> %s\n",
                client->ip,
                client->port,
                client->isAdmin ? "Admin" : "Observador",
                msg);
        fflush(logFile);
        pthread_mutex_unlock(&logMutex);
    }
}