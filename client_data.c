#include "client_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CLIENT_CAPACITY 10
#define INITIAL_SUBSCRIPTION_CAPACITY 5

void initClientSubscriptions(ClientSubscriptions* subscriptions) {
    subscriptions->number_of_subscriptions = 0;
    subscriptions->capacity = INITIAL_SUBSCRIPTION_CAPACITY;
    subscriptions->names = malloc(subscriptions->capacity * sizeof(char*));
    if (!subscriptions->names) {
        perror("Failed to allocate memory for subscription names");
        exit(EXIT_FAILURE);
    }
}

void addSubscription(ClientSubscriptions* subscriptions, const char* name) {
    if (subscriptions->number_of_subscriptions == subscriptions->capacity) {
        subscriptions->capacity *= 2;
        subscriptions->names = realloc(subscriptions->names, subscriptions->capacity * sizeof(char*));
        if (!subscriptions->names) {
            perror("Failed to reallocate memory for subscription names");
            exit(EXIT_FAILURE);
        }
    }
    subscriptions->names[subscriptions->number_of_subscriptions] = strdup(name);
    if (!subscriptions->names[subscriptions->number_of_subscriptions]) {
        perror("Failed to duplicate subscription name");
        exit(EXIT_FAILURE);
    }
    subscriptions->number_of_subscriptions++;
}

void initClientArray(ClientArray* clientArray) {
    clientArray->number_of_clients = 0;
    clientArray->capacity = INITIAL_CLIENT_CAPACITY;
    clientArray->clients = malloc(clientArray->capacity * sizeof(Client));
    if (!clientArray->clients) {
        perror("Failed to allocate memory for clients");
        exit(EXIT_FAILURE);
    }
}

void addClient(ClientArray* clientArray, const char* client_name, int current_socket) {
    if (clientArray->number_of_clients == clientArray->capacity) {
        clientArray->capacity *= 2;
        clientArray->clients = realloc(clientArray->clients, clientArray->capacity * sizeof(Client));
        if (!clientArray->clients) {
            perror("Failed to reallocate memory for clients");
            exit(EXIT_FAILURE);
        }
    }
    clientArray->clients[clientArray->number_of_clients].client_name = strdup(client_name);
    if (!clientArray->clients[clientArray->number_of_clients].client_name) {
        perror("Failed to duplicate client name");
        exit(EXIT_FAILURE);
    }
    clientArray->clients[clientArray->number_of_clients].is_online = 1;
    clientArray->clients[clientArray->number_of_clients].current_socket = current_socket;
    initClientSubscriptions(&clientArray->clients[clientArray->number_of_clients].subscriptions);
    clientArray->number_of_clients++;
}
