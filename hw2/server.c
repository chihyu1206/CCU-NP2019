#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <ctype.h>

void *do_chat(void *);
int PushClient(int);
int PopClient(int);

pthread_t thread;
pthread_mutex_t mutex;

#define MAX_CLIENT 10
#define CHATDATA 1024
#define MAX_NAME 16
#define INVALID_SOCK -1

struct user {
  int clientsocket;
  char name[MAX_NAME];
};
struct user userlist[MAX_CLIENT];

char LOGOUT[] = "!exit";
char INFO[] =
    "====================Welcome to Chating Room====================\n"
    "[To check all users       ] !users\n"
    "[To check your username   ] !whoami\n"
    "[To change your username  ] !rename:(New Username)\n"
    "[To send public message   ] (message)\n"
    "[To send private message  ] !(username)! (message)\n"
    "[To exit from the client  ] !exit\n" "Press Enter to continue!\n";
char Connected_Limit_Warning[] = "Sorry!!No More Connection!!\n";
char User_Not_Found[] = "This user is not online.Try again!!\n";

int main(int argc, char *argv[]) {
  int c_socket, s_socket;
  struct sockaddr_in s_addr, c_addr;
  int len, i, j, n, ret;

  if (argc < 2) {
    printf("format: %s port_number\n", argv[0]);
    exit(-1);
  }

  if (pthread_mutex_init(&mutex, NULL) != 0) {  /* Initialize a mutex.  */
    printf("Can not create mutex\n");
    return -1;
  }

  s_socket = socket(PF_INET, SOCK_STREAM, 0);

  memset(&s_addr, 0, sizeof(s_addr));
  s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  s_addr.sin_family = AF_INET;
  s_addr.sin_port = htons(atoi(argv[1]));
  /* Give the socket FD the local address ADDR (which is LEN bytes long).  */
  if (bind(s_socket, (struct sockaddr *) &s_addr, sizeof(s_addr)) == -1) {
    printf("Can not Bind\n");
    return -1;
  }
  /* Prepare to accept connections on socket FD.
   N connection requests will be queued before further requests are refused.
   Returns 0 on success, -1 for errors.  */
  if (listen(s_socket, MAX_CLIENT) == -1) {
    printf("listen Failed\n");
    return -1;
  }

  memset(&userlist, 0, sizeof(userlist));
  for (i = 0; i < MAX_CLIENT; i++)
    userlist[i].clientsocket = INVALID_SOCK;

  while (1) {
    len = sizeof(c_addr);
    c_socket = accept(s_socket, (struct sockaddr *) &c_addr, &len);

    ret = PushClient(c_socket);
    if (ret < 0) {
      write(c_socket, Connected_Limit_Warning,
            strlen(Connected_Limit_Warning));
      close(c_socket);
    } else {
      write(c_socket, INFO, strlen(INFO));
      pthread_create(&thread, NULL, do_chat, (void *) c_socket);
    }
  }
  return 0;
}

void *do_chat(void *arg)
{
  int c_socket = (int) arg;
  char chatData[CHATDATA];
  char sendBuffer[CHATDATA];
  int i, n;
  int SocketIndex;
  int userlen;
  int spaceBar = 1;

  char *message, *ptr;
  int length;

  //Find its index and register its name
  if ((n = read(c_socket, chatData, sizeof(chatData))) > 0) {
    ptr = strtok(chatData, "[]");
    userlen = strlen(ptr) + 2;  // 2 for [] (square brackets)
    for (i = 0; i < MAX_CLIENT; i++) {
      if (userlist[i].clientsocket == c_socket) {
        SocketIndex = i;
        strcpy(userlist[i].name, ptr);
        break;
      }
    }
  }

  while (1) {
    memset(chatData, 0, sizeof(chatData));
    memset(sendBuffer, 0, sizeof(sendBuffer));
    if ((n = read(c_socket, chatData, sizeof(chatData))) > 0) {
      //At the end of the data, there's a newline (\n, ASCII code 10)
      //A space (ASCII code 32) after [username]
      message = chatData + userlen + spaceBar;  //message points the pure message itself

      // username modify
      if (!strncmp(message, "!rename:", 8)) {
        ptr = message + 8;      //pointer for new user name

        if (strlen(ptr) >= MAX_NAME) {
          sprintf(sendBuffer,
                  "Length of username must be shorter than %d\n",
                  MAX_NAME);
        } else {                //Copy from chatData to userlist
          length = 0;
          while (*ptr) {
            if (*ptr == '\r')
              break;
            else if (*ptr == '\n')
              break;
            else if (length + 1 == MAX_NAME)
              break;
            else if (*ptr == ' ') {
              ptr++;
              continue;
            }
            userlist[SocketIndex].name[length] = *ptr;
            ptr++;
            length++;
          }
          userlist[SocketIndex].name[length] = '\0';
          sprintf(sendBuffer, "Your username is updated: %s\n",
                  userlist[SocketIndex].name);
        }
        write(c_socket, sendBuffer, sizeof(sendBuffer));
        continue;

      } else if (!strncmp(message, "!whoami", 7)) { // Check my own's name.
        sprintf(sendBuffer, "Your username: %s\n",
                userlist[SocketIndex].name);
        write(c_socket, sendBuffer, sizeof(sendBuffer));
        continue;

      } else if (!strncmp(message, "!users", 6)) { // Check All Online Users
        sprintf(sendBuffer, "======List of All Online Users======\n");
        write(c_socket, sendBuffer, sizeof(sendBuffer));
        for (i = 0, n = 0; i < MAX_CLIENT; i++) {
          if (userlist[i].clientsocket != INVALID_SOCK) {
            memset(sendBuffer, 0, sizeof(sendBuffer));
            sprintf(sendBuffer, "%d : %s\n", ++n, userlist[i].name);
            write(c_socket, sendBuffer, sizeof(sendBuffer));
          }
        }
        continue;
      //private message function
      } else if (message[0] == '!' &&     // first charater == '!'
               n >= userlen + 4 &&      // length equal or greater than 4 (!char!'sp')
               isalnum(message[1]) &&   // character or number right after slash
               strchr(&message[2], '!')) {      // after that a '!' exists
        ptr = strtok(message, "!");     //destination username

        //find socket from userlist that matches username
        for (i = 0; i < MAX_CLIENT; i++) {
          if (userlist[i].clientsocket != INVALID_SOCK
              && !strcmp(ptr, userlist[i].name)) {
            //write on socket (username)
            ptr = strtok(NULL, "");     //message
            sprintf(sendBuffer, "Private Message From %s: %s",
                    userlist[SocketIndex].name, ptr);
            write(userlist[i].clientsocket, sendBuffer, sizeof(sendBuffer));
            break;
          }
        }
        //if username not found
        if (i == MAX_CLIENT) {
          write(c_socket, User_Not_Found, sizeof(User_Not_Found));
          continue;
        }
      } else {
        sprintf(sendBuffer, "[%s] %s", userlist[SocketIndex].name, message);
        for (i = 0; i < MAX_CLIENT; i++) {
          if (userlist[i].clientsocket != INVALID_SOCK)
            write(userlist[i].clientsocket, sendBuffer, sizeof(sendBuffer));
        }
      }

      if (strstr(chatData, LOGOUT) != NULL) {
        PopClient(c_socket);
        break;
      }
    }
  }
}

int PushClient(int c_socket) {
  int i;

  for (i = 0; i < MAX_CLIENT; i++) {
    pthread_mutex_lock(&mutex);

    if (userlist[i].clientsocket == INVALID_SOCK) {
      userlist[i].clientsocket = c_socket;
      pthread_mutex_unlock(&mutex);
      return i;
    }
    pthread_mutex_unlock(&mutex);
  }
  if (i == MAX_CLIENT)
    return -1;
}

int PopClient(int s) {
  int i;
  close(s);
  for (i = 0; i < MAX_CLIENT; i++) {
    pthread_mutex_lock(&mutex);
    if (s == userlist[i].clientsocket) {
      userlist[i].clientsocket = INVALID_SOCK;
      pthread_mutex_unlock(&mutex);
      break;
    }
    pthread_mutex_unlock(&mutex);
  }
  return 0;
}
