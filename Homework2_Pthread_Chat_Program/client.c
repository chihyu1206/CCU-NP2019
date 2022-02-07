#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>

#define CHATDATA 2048
#define BUF_SIZE 1024
#define MAX_NAME 16

void *Send(void *);
void *Receive(void *);

pthread_t thread_1, thread_2;

char LOGOUT[] = "!exit";
char RENAME[] = "nickname:";
char nickname[MAX_NAME];

int main(int argc, char *argv[])
{
    int c_socket;
    struct sockaddr_in c_addr; /* Structure describing an Internet socket address.  */

    if (argc < 3) {
        printf("Format : %s ip_address port_number\n", argv[0]);
        exit(-1);
    }
    /* Create a new socket of type TYPE in domain DOMAIN, using
       protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
       Returns a file descriptor for the new socket, or -1 for errors.  
    */
    c_socket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&c_addr, 0, sizeof(c_addr));
    c_addr.sin_addr.s_addr = inet_addr(argv[1]);
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(atoi(argv[2]));

    printf("Input Nickname : ");
    scanf("%s", nickname);
    /* Open a connection on socket FD to peer at ADDR (which LEN bytes long).
       For connectionless socket types, just set the default address to send to
       and the only address from which to accept transmissions.
       Return 0 on success, -1 for errors.

       This function is a cancellation point and therefore not marked with
       __THROW.  
    */
    if (connect(c_socket, (struct sockaddr *)&c_addr, sizeof(c_addr)) == -1) {
        printf("Can not connect\n");
        return -1;
    }
    /* Create a new thread, starting with execution of START-ROUTINE
       getting passed ARG.  Creation attributed come from ATTR.  The new
       handle is stored in *NEWTHREAD.  
    */
    pthread_create(&thread_1, NULL, Send, &c_socket);
    pthread_create(&thread_2, NULL, Receive, &c_socket);
  
    /* Make calling thread wait for termination of the thread TH.  The
       exit status of the thread is stored in *THREAD_RETURN, if THREAD_RETURN
       is not NULL.

       This function is a cancellation point and therefore not marked with
       __THROW.  
    */
    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);

    close(c_socket);
    return 0;
}

void *Send(void *arg) {
    char chatData[CHATDATA];
    char buf[BUF_SIZE];
    int n;
    int c_socket = *(int *)arg;

    while (1) {
        memset(buf, 0, sizeof(buf));
        if ((n = read(0, buf, sizeof(buf))) > 0) {
            sprintf(chatData, "[%s]:%s", nickname, buf);
            write(c_socket, chatData, strlen(chatData));

            if (strncmp(buf, LOGOUT, strlen(LOGOUT)) == 0) {
                /* The pthread_kill() function sends the signal sig to thread, a thread
                    in the same process as the caller.  The signal is asynchronously
                    directed to thread.

                    If sig is 0, then no signal is sent, but error checking is still
                    performed.
                */
                pthread_kill(thread_2, SIGINT);
                break;
            }
        }   
    }
}

void *Receive(void *arg) {
    char chatData[CHATDATA];
    int n;
    int c_socket = *(int *)arg;
    int i = 0;

    while (1) {
        memset(chatData, 0, sizeof(chatData));
        if ((n = read(c_socket, chatData, sizeof(chatData))) > 0) {
            write(1, chatData, n);
        }
    }
}
