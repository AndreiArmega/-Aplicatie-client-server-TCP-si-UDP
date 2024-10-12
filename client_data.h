#ifndef CLIENT_DATA_H
#define CLIENT_DATA_H

//array-ul dinamic de clienti cu subscriptii

typedef struct {
    char** names;
    int number_of_subscriptions;
    int capacity;
} ClientSubscriptions;

typedef struct {
    char* client_name;
    ClientSubscriptions subscriptions;
    int is_online;
    int current_socket;
} Client;

typedef struct {
    Client* clients;
    int number_of_clients;
    int capacity;
} ClientArray;

void initClientSubscriptions(ClientSubscriptions* subscriptions);
void addSubscription(ClientSubscriptions* subscriptions, const char* name);
void initClientArray(ClientArray* clientArray);
void addClient(ClientArray* clientArray, const char* client_name, int current_socket);

#endif
