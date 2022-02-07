#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define PORT "80"
#define RESTRICT 10

char website[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<head>\r\n"
"<meta name=\"viewport\" content=\"width=device-width, minimum-scale=0.1\">\r\n"
"<title>0Dw4dCP.png (1200×1678)</title></head><body style=\"margin: 0px; background: #0e0e0e;\">\r\n"
"<img style=\"-webkit-user-select: none;cursor: zoom-in;\" src=\"https://i.imgur.com/0Dw4dCP.png\" width=\"379\" height=\"530\">\r\n"
"</body></html>\r\n"; /*html website*/

int main(void) {
    int status;
    fd_set main_fds, read_fds;  //main fd list && temporary fd list
    FD_ZERO(&main_fds); FD_ZERO(&read_fds);
    char buf[4096];
    struct addrinfo hints;
    struct addrinfo *servinfo, *ptr; // point to the result and a addrinfo pointer
    memset(&hints, 0, sizeof(hints)); // ensure the struct is empty
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill up my IP 
    /* Translate name of a service location and/or a service name to set of
        socket addresses.

        This function is a possible cancellation point and therefore not
        marked with __THROW.  
    */
    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    int listen_fd, optval = 1;
    for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
        /* Create a new socket of type TYPE in domain DOMAIN, using
        protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
        Returns a file descriptor for the new socket, or -1 for errors.  
        */
        if ((listen_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0) { //set socket
            perror("socket error:");
            continue;
        }
        /* Set socket FD's option OPTNAME at protocol level LEVEL
            to *OPTVAL (which is OPTLEN bytes long).
            Returns 0 on success, -1 for errors.  
        */
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) { //set socket option
            perror("set socket option error:");
            exit(1);
        }
        /* Give the socket FD the local address ADDR (which is LEN bytes long).  */
        if (bind(listen_fd, ptr->ai_addr, ptr->ai_addrlen) < 0) {
            perror("bind error:");
            close(listen_fd);
            continue;
        }
        break;
    }
    if (ptr == NULL) {  //failed to bind
        fprintf(stderr, "elect_server: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(servinfo);
    /* Prepare to accept connections on socket FD.
       N connection requests will be queued before further requests are refused.
       Returns 0 on success, -1 for errors.  
    */
    if (listen(listen_fd, RESTRICT) == -1) {
        perror("listen error");
        exit(3);
    }
    FD_SET(listen_fd, &main_fds);
    int max_fd = listen_fd;

    while (1) {
        fd_set read_fds = main_fds;
        struct timeval tv;
        tv.tv_sec = 5; tv.tv_usec = 0;
        int ret = 0, i = 0;
        /* Check the first NFDS descriptors each in READFDS (if not NULL) for read
           readiness, in WRITEFDS (if not NULL) for write readiness, and in EXCEPTFDS
           (if not NULL) for exceptional conditions.  If TIMEOUT is not NULL, time out
            after waiting the interval specified therein.  Returns the number of ready
            descriptors, or -1 for errors.

            This function is a cancellation point and therefore not marked with
            __THROW.  
        */
        if ((ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv)) == -1) {
            perror("select error:");
            exit(4);
        } else if (ret == 0) {
            printf("Time out!\n");
            continue;
        } else {
            for (i = 0; i < max_fd; i++) {
                if (FD_ISSET(i, &read_fds)) {
                    if (i == listen_fd) {  //new connected
                        struct sockaddr_in client_addr;
                        int new_fd;
                        socklen_t socklen = sizeof(struct sockaddr_in);
                        /* Await a connection on socket FD.
                        When a connection arrives, open a new socket to communicate with it,
                        set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
                        peer and *ADDR_LEN to the address's actual length, and return the
                        new socket's descriptor, or -1 for errors.

                        This function is a cancellation point and therefore not marked with
                        __THROW.  
                        */
                        if ((new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &socklen) == -1)) {
                            perror("accept error:");
                            exit(5);
                        } else {
                            memset(buf, '\0', sizeof(buf));
                            read(new_fd, buf, sizeof(buf) - 1);
                            printf("New connection comes from %s on PORT:%s by fd:%d\n", 
                            inet_ntoa(client_addr.sin_addr), PORT, new_fd);
                            write(new_fd, website, sizeof(website) - 1);
                        }
                    } else { // already connected
                        int recv_len;
                        memset(buf, '\0', sizeof(buf));
                        //recv() is used to receive messages from a connected socket//
                        if ((recv_len = recv(i, buf, sizeof(buf), 0)) == -1) {
                            perror("recv error:");
                            exit(6);
                        } else if (recv_len == 0) {
                            printf("Disconnected\n");
                        } else {
                            printf("Receive message:%s\n", buf);
                            send(i, buf, recv_len, 0);
                        }
                        close(i);
                        FD_CLR(i, &main_fds);
                    }
                }
            }
        }
    } 
    return 0;
}
