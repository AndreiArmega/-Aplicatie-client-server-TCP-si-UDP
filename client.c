#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include "packets.h"
#include "socket_modifiers.h"

#define MAX_EVENTS 5
#define BUFFER_SIZE 10000

// parsam comanda de subscribe/unsubscribe
struct tcp_subscribe* parse_subscribe_command(const char *input) {
    if (input == NULL) {
        fprintf(stderr, "Error: Input is null\n");
        return NULL;
    }

    struct tcp_subscribe *command = malloc(sizeof(struct tcp_subscribe));
    if (command == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    const char *subscribe_prefix = "subscribe ";
    const char *unsubscribe_prefix = "unsubscribe ";
    size_t subscribe_prefix_len = strlen(subscribe_prefix);
    size_t unsubscribe_prefix_len = strlen(unsubscribe_prefix);

    if (strncmp(input, subscribe_prefix, subscribe_prefix_len) == 0) {
        command->type = 0;
        strncpy(command->topic, input + subscribe_prefix_len, sizeof(command->topic) - 1);
        command->topic[sizeof(command->topic) - 1] = '\0';
    } else if (strncmp(input, unsubscribe_prefix, unsubscribe_prefix_len) == 0) {
        command->type = 1;
        strncpy(command->topic, input + unsubscribe_prefix_len, sizeof(command->topic) - 1);
        command->topic[sizeof(command->topic) - 1] = '\0';
    } else {
        fprintf(stderr, "Error: Invalid command format\n");
        free(command);
        return NULL;
    }

    return command;
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    // stdout unbuffered
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* ip_server = argv[2];
    int port_server = atoi(argv[3]);
    int server_socket, epoll_fd;
    struct sockaddr_in server_addr;
    struct epoll_event event, events[MAX_EVENTS];

    // parte de creat socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_server);
    if (inet_pton(AF_INET, ip_server, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    make_socket_non_blocking(server_socket);
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // multiplexam doua tipuri de operatii => citire de la server si citire de la stdin
    event.events = EPOLLIN;
    event.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        perror("epoll_ctl: server_socket");
        close(server_socket);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }
    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event) == -1) {
        perror("epoll_ctl: stdin");
        close(server_socket);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // protocolul peste ipv4 incepe cu acest hello, explicat in readme
    char* client_id = argv[1];
    struct tcp_hello hello_packet;
    strncpy(hello_packet.name, client_id, sizeof(hello_packet.name));
    write(server_socket, &hello_packet, sizeof(hello_packet));

    while (1) {
        //astept
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            // primesc ceva de la server, vreau sa printez;
            if (events[i].data.fd == server_socket) {
                char buffer[BUFFER_SIZE];
                set_blocking_mode(server_socket);   
                int count = read(server_socket, buffer, BUFFER_SIZE);
                make_socket_non_blocking(server_socket);
                if (count == -1) {
                    perror("read");
                    break;
                } else if (count == 0) {
                    printf("Server closed the connection\n");
                    close(server_socket);
                    close(epoll_fd);
                    return 0;
                }
                buffer[count] = '\0';
                printf("%s", buffer);
            } else if (events[i].data.fd == STDIN_FILENO) {
                //daca citesc ceva de la stdin, va fi exit sau subscribe/unsubscribe
                char *msg = NULL;
                size_t len = 0;
                ssize_t nread;
                nread = getline(&msg, &len, stdin);
                if (nread > 0) {
                    if (msg[nread - 1] == '\n') {
                        msg[nread - 1] = '\0';
                        nread--;
                    }
                }
                //daca e exit inchidem socketii si return 0
                if (strncmp(msg, "exit", 4) == 0) {
                    close(server_socket);
                    close(epoll_fd);
                    return 0;
                }
                //parsam comanda
                struct tcp_subscribe* command = parse_subscribe_command(msg);
                if (command != NULL) {
                    write(server_socket, command, sizeof(struct tcp_subscribe));
                    char buffer[5];
                    //asteptam ack dupa ce am trimis o comanda
                    set_blocking_mode(server_socket);
                    int count = read(server_socket, buffer, 5);
                    make_socket_non_blocking(server_socket);
                    if(count == -1) {
                        perror("read");
                        break;
                    }
                    if(strncmp(buffer, "ACK", 3) != 0) {
                        printf("Error: Server did not acknowledge the command\n");
                        break;
                    }
                    if(command->type == 1)
                        printf("Unsubscribed from topic %s\n", command->topic);
                    else
                        printf("Subscribed to topic %s\n", command->topic);
                    free(command);
                }
            }
        }
    }

    close(server_socket);
    close(epoll_fd);
    return 0;
}
