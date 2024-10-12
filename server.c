#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "client_data.h"
#include "packets.h"
#include "socket_modifiers.h"

#define MAX_CLIENTS 30
#define MAX_EVENTS 35
#define BUFFER_SIZE 10000


/*
    Parser pentru mesajele trimise catre clienti
    A fost folosit la debugging
    Scoate informatiile precum ip si port din mesaj
*/
char *extract_info(const char *input) {
    const char *ptr = input;

    while (*ptr && *ptr != ':') {
        ptr++;
    }

    while (*ptr && !isdigit(*ptr)) {
        ptr++;
    }
    while (*ptr && isdigit(*ptr)) {
        ptr++;
    }
    while (*ptr && (*ptr == ' ' || *ptr == '-')) {
        ptr++;
    }
    if (*ptr) {
        char *result = (char *)malloc(strlen(ptr) + 2);
        if (result) {
            strcpy(result, ptr);
            return result;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}


/*
    Primeste un topic in forma de path si scoate o lista de stringuri
    care sunt, de fapt, bucati din path; va intoarce si numarul de bucati
*/
char** split_topic(const char* topic, int* count) {
    if (!topic || !count) return NULL;

    *count = 1;
    for (const char* p = topic; *p; p++) {
        if (*p == '/') (*count)++;
    }

    char** parts = malloc(*count * sizeof(char*));
    if (!parts) return NULL;

    const char* start = topic;
    int i = 0;
    while (*start) {
        const char* end = strchr(start, '/');
        if (!end) end = start + strlen(start);

        int len = end - start;
        parts[i] = malloc(len + 1);
        if (!parts[i]) {
            while (i > 0) free(parts[--i]);
            free(parts);
            return NULL;
        }

        strncpy(parts[i], start, len);
        parts[i][len] = '\0';
        start = *end ? end + 1 : end;
        i++;
    }

    return parts;
}

/*
    Functie care primeste 2 stringuri si verifica daca sunt compatibile,
    conform definitiei wildcardurilor
*/
int finite_state(char *str1, char *str2) {
    int count1 = 0, count2 = 0;
    char **segments1 = split_topic(str1, &count1);
    char **segments2 = split_topic(str2, &count2);
    int i = 0, j = 0;
    int finished = 0;
    while (!finished) {
        if (i < count1 && j < count2) {
            if (strcmp(segments1[i], segments2[j]) == 0) {
                i++;
                j++;
            } else if (strcmp(segments1[i], "+") == 0) {
                i++;
                j++;
            } else if (strcmp(segments1[i], "*") == 0) {
                if (i == count1 - 1) {
                    i++;
                    j = count2;
                } else {
                    int found = 0;
                    while (j < count2 && !found) {
                        if (strcmp(segments1[i + 1], segments2[j]) != 0) {
                            j++;
                        } else {
                            found = 1;
                        }
                    }
                    if (found) i++;
                    else finished = 1;
                }
            } else {
                finished = 1;
            }
        } else {
            finished = 1;
        }
    }

    int match = (i == count1 && j == count2);
    for (int k = 0; k < count1; k++) free(segments1[k]);
    for (int k = 0; k < count2; k++) free(segments2[k]);
    free(segments1);
    free(segments2);

    return match;
}
// parseaza un buffer si extrage informatiile in structura udp_message
// switch-ul din interior ia fiecare caz si copiaza un numar de bytes
// predefinit, sau exact cati byti trebuie in cazul de string
void parse_buffer_udp(const char *buffer, udp_message *msg) {

    int i = 0;
    while(buffer[i]) {
        i++;
    }
    strncpy(msg->topic, buffer, 50);
    msg->topic[50] = '\0';
    char *ptr = buffer + 50;
    msg->tip_date = (unsigned char) (*(unsigned char *)(ptr));
    ptr++;
    switch (msg->tip_date) {
        case 0: {
            memcpy(msg->continut, ptr, 5);
            break;
        }
        case 1: {
            memcpy(msg->continut, ptr, 5);
            break;
        }
        case 2 : {
            memcpy(msg->continut, ptr, 7);
            break;

        }
        default:
            int j = 0;
            while(ptr[j]) {
                msg->continut[j] = ptr[j];
                j++;
            }
            break;
    }
}

// functie care ia o structura udp_message si stie sa genereze mesajul pentru
// client; functia intoarce mesajul cu tot cu date precum ip si port, insa acestea
// sunt parsate la final pentru checker; au fost folosite in debugging
char *udp_message_to_client(const udp_message *msg, struct sockaddr_in c) {
    char *for_client = calloc(10000 * sizeof(char), 1);    
    switch (msg->tip_date) {
        case 0: {
            unsigned char byte = (unsigned char) (*(unsigned char *)(msg->continut));
            uint32_t value = (uint32_t)(*(uint32_t *)(msg->continut + 1));
            if(byte == 0 || htonl(value) == 0)
                sprintf(for_client, "%s:%d - %s - INT - %u", inet_ntoa(c.sin_addr), 
                                                    ntohs(c.sin_port), 
                                                    msg->topic,
                                                    htonl(value));
            else
                sprintf(for_client, "%s:%d - %s - INT - -%u", inet_ntoa(c.sin_addr), 
                                                    ntohs(c.sin_port), 
                                                    msg->topic,
                                                    htonl(value));
            break;
        }
        case 1: {
            uint16_t value = (uint32_t)(*(uint32_t *)(msg->continut));
            sprintf(for_client, "%s:%d - %s - SHORT_REAL - %.2f", inet_ntoa(c.sin_addr), 
                                                ntohs(c.sin_port), 
                                                msg->topic,
                                                (float)((float)(htons(value))/100.00));
            break;
        }
        case 2 : {
            unsigned char byte = (unsigned char) (*(unsigned char *)(msg->continut));
            uint32_t value = (uint32_t)(*(uint32_t *)(msg->continut + 1));
            uint8_t decimal = (uint8_t)(*(uint8_t *)(msg->continut + 5));
            if(byte == 0)
                sprintf(for_client, "%s:%d - %s - FLOAT - %.4f", inet_ntoa(c.sin_addr), 
                                                    ntohs(c.sin_port), 
                                                    msg->topic,
                                                    htonl(value) * pow(10, -decimal));
            else
                sprintf(for_client, "%s:%d - %s - FLOAT - -%.4f", inet_ntoa(c.sin_addr), 
                                                    ntohs(c.sin_port), 
                                                    msg->topic,
                                                    htonl(value) * pow(10, -decimal));
            break;

        }
        default:
            sprintf(for_client, "%s:%d - %s - STRING - %s", inet_ntoa(c.sin_addr), 
                                                ntohs(c.sin_port), 
                                                msg->topic,
                                                msg->continut);
            break;
    }
    return for_client;
}


// functie dummy
void print_udp_message(const udp_message *msg) {
    ;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // facem stdout unbuffered
    setvbuf(stdout, NULL, _IONBF, 0);
    
    //declaram structurile necesare
    int tcp_fd, udp_fd, new_socket, epoll_fd;
    struct sockaddr_in address, client_addr;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    int port = atoi(argv[1]);
    udp_message udp_msg;
    memset(&udp_msg, 0, sizeof(udp_msg));
    ClientArray clientArray;
    initClientArray(&clientArray);

    struct epoll_event ev, events[MAX_EVENTS];

    // tcp
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (tcp_fd < 0 || udp_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // setam non-blocking
    fcntl(tcp_fd, F_SETFL, O_NONBLOCK);
    fcntl(udp_fd, F_SETFL, O_NONBLOCK);

    // setsockopt
    if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))
        || setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(tcp_fd, (struct sockaddr *)&address, sizeof(address)) < 0
        || bind(udp_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // listen pe tcp
    listen(tcp_fd, 3);

    // EPOLL - Edge Triggered
    // multiplexam 3 tipuri de operatii: citirea de la tastatura,
    // socket-ul tcp si socket-ul udp
    // in functie de numarul de clienti, se va multiplexa si interactiunea
    // cu fiecare client
    // EPOLL ET => performanta mai buna fata de select
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = tcp_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_fd, &ev);

    ev.data.fd = udp_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_fd, &ev);

    struct epoll_event ev_stdin;
    ev_stdin.events = EPOLLIN;
    ev_stdin.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev_stdin) == -1) {
        perror("epoll_ctl: stdin");
        exit(EXIT_FAILURE);
    }

ev_loop:
    while (1) {
        //astept
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int n = 0; n < nfds; ++n) {
            //s-a scris la tastatura ceva
            if (events[n].data.fd == STDIN_FILENO) {
                char cmd_buffer[100];
                //verificam daca s-a dat exit, altfel nu se intampla nimic
                //in caz de exit se va iesi pur simplu, dupa ce inchidem socketii
                if (fgets(cmd_buffer, sizeof(cmd_buffer), stdin) != NULL) {
                    if (strncmp(cmd_buffer, "exit\n", 5) == 0) {
                        printf("Exit command received, shutting down...\n");
                        for (int i = 0; i < clientArray.number_of_clients; i++) {
                            close(clientArray.clients[i].current_socket);
                            clientArray.clients[i].is_online = 0;
                        }
                        close(tcp_fd);
                        close(udp_fd);
                        close(epoll_fd);
                        exit(0);
                    }
                }
            }
            //s-a primit o conexiune noua
            if (events[n].data.fd == tcp_fd) {
                addrlen = sizeof(client_addr);
                // aici incepe protocolul propriu-zis peste tcp
                // atunci cand se primeste o conexiune noua, se asteapta un mesaj
                // de hello de la client; apoi, serverul ce va face
                while ((new_socket = accept(tcp_fd, (struct sockaddr *)&client_addr, &addrlen)) != -1) {
                    fcntl(new_socket, F_SETFL, O_NONBLOCK);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = new_socket;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev);
                    int flag = 1;
                    int result = setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
                    if (result < 0) {
                        perror("setsockopt(TCP_NODELAY) failed");
                        close(new_socket);
                        exit(EXIT_FAILURE);
                    }
                    //primesc hello
                    struct tcp_hello hello_msg;
                    ssize_t len = read(new_socket, &hello_msg, sizeof(hello_msg));
                    if (len > 0) {
                        //verific daca acest client exista in array-ul meu de clienti
                        int is_in_array = 0;
                        for (int i = 0; i < clientArray.number_of_clients; i++) {
                            if (strcmp(clientArray.clients[i].client_name, hello_msg.name) == 0) {
                                is_in_array = 1;
                                //daca este deja online, inchid conexiunea cu noul client intrat cu
                                //acelasi nume
                                if (clientArray.clients[i].is_online == 1) {
                                    printf("Client %s already connected.\n", hello_msg.name);
                                    close(new_socket);
                                    goto ev_loop;
                                } else {
                                    //daca a mai intrat si nu este online, il fac online
                                    //actualizez socket
                                    clientArray.clients[i].is_online = 1;
                                    clientArray.clients[i].current_socket = new_socket;
                                }
                            }
                        }
                        //daca nu a mai fost niciodata conectat, voi crea un nou client
                        if (is_in_array == 0) {
                            addClient(&clientArray, hello_msg.name, new_socket);
                        }
                        char *ip_str = inet_ntoa(client_addr.sin_addr);
                        int port = ntohs(client_addr.sin_port);
                        printf("New client %s connected from %s:%d\n", (&hello_msg)->name, 
                                                                        ip_str,
                                                                        port);
                    }
                    addrlen = sizeof(client_addr);
                }
            } else if (events[n].data.fd == udp_fd) {
                // primesc un packet udp
                ssize_t len = 0;
                while(1) {
                    //EPOLLET => dau receive pana epuiez
                    len = recvfrom(udp_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addrlen);
                    if (len == -1) {
                        // No more data to read, break the loop
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;  // Normal exit from non-blocking recvfrom
                        } else {
                            perror("recvfrom failed");  // An actual error occurred
                            exit(EXIT_FAILURE);
                        }
                    }
                    //parsam mesajul si incercam sa il trimitem clientilor
                    if (len > 0) {
                        parse_buffer_udp(buffer, &udp_msg);
                        print_udp_message(&udp_msg);                        
                        for (int i = 0; i < clientArray.number_of_clients; i++) {
                            for (int j = 0; j < clientArray.clients[i].subscriptions.number_of_subscriptions; j++) {
                                if (strcmp(clientArray.clients[i].subscriptions.names[j], udp_msg.topic) == 0 ||
                                    finite_state(clientArray.clients[i].subscriptions.names[j], udp_msg.topic) == 1){
                                    if (clientArray.clients[i].is_online == 1) {
                                        //iau mesajul si extrag info
                                        char *msg = udp_message_to_client(&udp_msg, client_addr);
                                        char *info = extract_info(msg);
                                        free(msg);
                                        if (info == NULL) {
                                            perror("extract_info");
                                            exit(EXIT_FAILURE);
                                        }
                                        strcat(info, "\n");
                                        //trimit mesajul
                                        write(clientArray.clients[i].current_socket, info, strlen(info));
                                        break;
                                    }
                                }
                            }
                        }
                        //flush pe udp_msg
                        memset(&udp_msg, 0, sizeof(udp_msg));
                    }
                }
                
            } else {
                // comunicatia cu clientii tcp
                // singurul tip de comunicatie este subscribe/unsubscribe
                ssize_t len = read(events[n].data.fd, buffer, BUFFER_SIZE);
                if (len > 0) {
                    //primim pachet de subscribe
                    struct tcp_subscribe *subscribe = (struct tcp_subscribe *) buffer;
                    //daca este subscribe, adaugam topicul la lista de topicuri
                    for (int i = 0; i < clientArray.number_of_clients; i++) {
                        if (clientArray.clients[i].current_socket == events[n].data.fd) {
                            if (subscribe->type == 0) {
                                addSubscription(&clientArray.clients[i].subscriptions, subscribe->topic);
                                //printf("Client %s subscribed to topic %s.\n", clientArray.clients[i].client_name, subscribe->topic);
                                //protocolul trimite si un ACK
                                char ack[5] = "ACK";
                                write(events[n].data.fd, ack, 5);
                            } else {
                                //unsubscribe => remove din lista
                                for (int j = 0; j < clientArray.clients[i].subscriptions.number_of_subscriptions; j++) {
                                    if (strcmp(clientArray.clients[i].subscriptions.names[j], subscribe->topic) == 0) {
                                        free(clientArray.clients[i].subscriptions.names[j]);
                                        for (int k = j; k < clientArray.clients[i].subscriptions.number_of_subscriptions - 1; k++) {
                                            clientArray.clients[i].subscriptions.names[k] = clientArray.clients[i].subscriptions.names[k + 1];
                                        }
                                        clientArray.clients[i].subscriptions.number_of_subscriptions--;
                                        //printf("Client %s unsubscribed from topic %s.\n", clientArray.clients[i].client_name, subscribe->topic);
                                        //ack
                                        char ack[5] = "ACK";
                                        write(events[n].data.fd, ack, 5);
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }

        

                } else if (len == 0 || (len < 0 && (errno == ECONNRESET || errno == EPIPE))) {
                    // cazul de inchidere conexiuni, inchidem socketul si il scoatem din epoll
                    // apoi afisam un mesaj
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
                    close(events[n].data.fd);

                    //vedem cine s-a deconectat

                    for(int i = 0; i < clientArray.number_of_clients; i++) {
                        if(clientArray.clients[i].current_socket == events[n].data.fd) {
                            printf("Client %s disconnected.\n", clientArray.clients[i].client_name);
                            clientArray.clients[i].is_online = 0;
                            break;
                        }
                    }

                    // de asemenea, clientul nu este sters, doar va fi marcat ca offline
                    // acesta se poate reconecta si trebuie sa ii pastram topicurile
                    for (int i = 0; i < clientArray.number_of_clients; i++) {
                        if (clientArray.clients[i].current_socket == events[n].data.fd) {
                            clientArray.clients[i].is_online = 0;
                            break;
                        }
                    }
                }
            }
        }
    }

    close(tcp_fd);
    close(udp_fd);
    close(epoll_fd);

    return 0;
}