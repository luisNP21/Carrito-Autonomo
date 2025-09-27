#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>


typedef struct Client Client;
// Envía un mensaje simple a un socket (sin log)
void sendMessage(int sock, const char *msg);

// Envía un mensaje a un cliente y lo guarda en el log
void sendMessageToClient(Client *client, const char *msg);

#endif
