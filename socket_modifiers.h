// socket_modes.h
#ifndef SOCKET_MODES_H
#define SOCKET_MODES_H

#include <fcntl.h> 

// seteaza non-blocking mode pentru un socket
int make_socket_non_blocking(int sfd);

// seteaza blocking mode pentru un socket
void set_blocking_mode(int socket_fd);

#endif
