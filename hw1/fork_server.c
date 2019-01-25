#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
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

void sigchld_handler(int s)
{
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

int main(int argc, char *argv[]) {
	struct sockaddr_in server_addr, client_addr;  /*server && client's address*/
	socklen_t addr_len = sizeof(client_addr);
	int server_fd, client_fd;
	char buf[4096];                /*use to store infomation from client*/

  int status;
  struct addrinfo hints;
  struct addrinfo *servinfo, *ptr; // point to the result and a addrinfo pointer
  memset(&hints, 0, sizeof(hints)); // ensure the struct is empty
  hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE; // fill up my IP 
  if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }
  int optval = 1;
  for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {  //run a for loop to chect which is able to use
    if ((server_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) { //set socket
      perror("server: socket");
      continue;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) { //set socket option
      perror("setsockopt");
      exit(1);
    }

    if (bind(server_fd, ptr->ai_addr, ptr->ai_addrlen) == -1) { //set port number to defined PORT in getaddrinfo 
      perror("server: bind");
      close(server_fd);
      continue;
    }
    break;
  }
  if (ptr == NULL) {  //nothing is eligible
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }
  freeaddrinfo(servinfo); // 全部都用這個 structure
  if (listen(server_fd, RESTRICT) == -1) {  //listen at port 80, the limit is RESTRICT(a constant)
    perror("listen");
    exit(1);
  }
  /*deal with the zombie process using the waitpid(),
  then the parent process have to wait until the child process end.
  Thus, the kernel can get the info and release the pid*/
  struct sigaction sa;             
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("Server: Waiting for connections...\n");
  /*main loop to accept*/
  int pid;
	while(1) {
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

		if(client_fd == -1) {
			perror("Connection failed\n");
			continue;
		}
    printf("Got client connection from %s\n", inet_ntoa(client_addr.sin_addr));
		pid = fork();
		if (pid == 0) {
			/*child process */
			close(server_fd);
			memset(buf, '\0', sizeof(buf));
			read(client_fd, buf, sizeof(buf) - 1);

			printf("%s\n", buf);

			write(client_fd, website, sizeof(website) -1);
      close (client_fd);
			printf("Closing the client_fd\n");
			exit(0);
		}
		/*parent process*/
		if (pid > 0) {
			wait(NULL);
			close(client_fd)
		}
	}
	return 0;
}
