// Copyright (c) 2025 All Rights Reserved.
#include "./server.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define NOTIFY_ADDR "matt.d.oliveira@gmail.com"
#define ADMIN_NAME "@Matt"

static void send_welcome(int fd) {
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  char buf[MAX_MSG_LEN];
  int off = strftime(buf, sizeof buf, "[%H:%M] ", tm);
  snprintf(buf + off, sizeof buf - off,
           "Welcome to m-chat! Use /help for help.\n");
  send(fd, buf, strlen(buf), 0);
}

static void notify_connect(void) {
  pid_t pid = fork();
  if (pid != 0)
    return;
  FILE *fp = popen("/usr/sbin/sendmail " NOTIFY_ADDR, "w");
  if (!fp)
    _exit(1);
  fprintf(fp, "To: " NOTIFY_ADDR "\n");
  fprintf(fp, "Subject: m-chat: new connection\n\n");
  fprintf(fp, "A user has connected to m-chat.\n");
  pclose(fp);
  _exit(0);
}

static void broadcast_userlist(client_list_t *cl) {
  char buf[MAX_SEND_LEN];
  int off = snprintf(buf, sizeof buf, "%s", OPT_MSG_USERLIST);
  for (int i = 1; i <= cl->count; i++) {
    if (i > 1)
      off += snprintf(buf + off, sizeof buf - off, ",");
    off += snprintf(buf + off, sizeof buf - off, "%s", cl->clients[i].name);
  }
  off += snprintf(buf + off, sizeof buf - off, "\n");
  for (int i = 1; i <= cl->count; i++)
    send(cl->fds[i].fd, buf, off, 0);
}

int main(int argc, char *argv[]) {
  int serv_fd;
  struct sockaddr_un serv_addr;
  client_list_t client_list;

  signal(SIGCHLD, SIG_IGN);

  serv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (serv_fd == -1) {
    perror("server: socket");
    exit(EXIT_FAILURE);
  }

  unlink(SOCK_PATH);
  memset(&serv_addr, 0, sizeof serv_addr);
  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, SOCK_PATH, sizeof(serv_addr.sun_path) - 1);

  if (bind(serv_fd, (struct sockaddr *)&serv_addr, sizeof serv_addr) == -1) {
    perror("server: bind");
    exit(EXIT_FAILURE);
  }

  if (listen(serv_fd, BACKLOG) == -1) {
    perror("server: listen");
    exit(EXIT_FAILURE);
  }

  memset(&client_list, 0, sizeof client_list);
  client_list.clients = malloc(MAX_CLIENTS * sizeof(client_t));
  client_list.fds = malloc(MAX_CLIENTS * sizeof(struct pollfd));

  client_list.fds[0].fd = serv_fd;
  client_list.fds[0].events = POLLIN;

  while (1) {
    int serv_offset = client_list.count + 1;
    int ready = poll(client_list.fds, serv_offset, -1);

    if (ready > 0) {
      if (client_list.fds[0].revents & POLLIN) {
        int client_fd = accept(serv_fd, NULL, NULL);

        client_t client;
        client.fd = client_fd;
        client_list.clients[serv_offset] = client;
        snprintf(client_list.clients[serv_offset].name, MAX_NAME_LEN, "%s",
                 "Anonymous");
        client_list.fds[serv_offset].fd = client_fd;
        client_list.fds[serv_offset].events = POLLIN;
        client_list.count++;

        fprintf(stdout, "Client connected: %d\n", client_fd);
        send_welcome(client_fd);
        notify_connect();
        broadcast_userlist(&client_list);
      }

      for (int i = 1; i < serv_offset; i++) {
        if (client_list.fds[i].revents & POLLIN) {
          char buf[MAX_MSG_LEN];
          ssize_t bytes_received;
          bytes_received = recv(client_list.fds[i].fd, buf, sizeof buf, 0);

          if (bytes_received > 0) {
            buf[bytes_received] = '\0';
            if (buf[0] == '/') {
              char *save_ptr;

              if (strcmp(buf, "/menu\n") == 0) {
                ssize_t opt_bytes_sent;
                opt_bytes_sent = send(client_list.fds[i].fd, OPT_MSG_MENU,
                                      strlen(OPT_MSG_MENU), 0);
                if (opt_bytes_sent <= 0) {
                  perror("server: /menu send");
                  continue;
                }
                continue;
              }
              if (strcmp(buf, "/help\n") == 0) {
                send(client_list.fds[i].fd, OPT_MSG_HELP,
                     strlen(OPT_MSG_HELP), 0);
                continue;
              }
              if (strncmp(buf, "/login ", 7) == 0) {
                char *pw = buf + 7;
                char *nl = strchr(pw, '\n');
                if (nl)
                  *nl = '\0';
                char *env_pw = getenv("MCHAT_PASSWORD");
                if (env_pw && strcmp(pw, env_pw) == 0) {
                  snprintf(client_list.clients[i].name, MAX_NAME_LEN, "%s",
                           ADMIN_NAME);
                  char msg[] = "Authenticated as " ADMIN_NAME "\n";
                  send(client_list.fds[i].fd, msg, strlen(msg), 0);
                  broadcast_userlist(&client_list);
                } else {
                  char msg[] = "Authentication failed\n";
                  send(client_list.fds[i].fd, msg, strlen(msg), 0);
                }
                continue;
              }
              if (strcmp(buf, "/exit\n") == 0) {
                ssize_t opt_bytes_sent;
                opt_bytes_sent = send(client_list.fds[i].fd, OPT_MSG_EXIT,
                                      strlen(OPT_MSG_EXIT), 0);
                if (opt_bytes_sent <= 0) {
                  perror("server: /exit send");
                  continue;
                }

                int closed_fd = client_list.fds[i].fd;
                client_list.fds[i] = client_list.fds[client_list.count];
                client_list.clients[i] = client_list.clients[client_list.count];

                close(closed_fd);
                client_list.count--;
                broadcast_userlist(&client_list);
                continue;
              }
              char *token = strtok_r(buf, DELIMITERS, &save_ptr);
              if (strcmp(token, "/nick") == 0) {
                token = strtok_r(NULL, DELIMITERS, &save_ptr);
                fprintf(stdout, "token = %s\n", token);
                if (token == NULL || token[0] == ' ') {
                  send(client_list.fds[i].fd, ERR_NICK_EMPTY,
                       strlen(ERR_NICK_EMPTY), 0);
                  fprintf(stdout, "nick empty sent");
                  continue;
                }
                if (strlen(token) > MAX_NAME_LEN) {
                  send(client_list.fds[i].fd, ERR_NICK_LEN,
                       strlen(ERR_NICK_LEN), 0);
                  fprintf(stdout, "nick len sent");
                  continue;
                }
                ssize_t opt_bytes_sent;
                char nick_msg[MAX_MSG_LEN];
                snprintf(client_list.clients[i].name, MAX_NAME_LEN, "%s",
                         token);
                snprintf(nick_msg, MAX_MSG_LEN, "%s %s\n", OPT_MSG_NICK_SUCCESS,
                         token);
                opt_bytes_sent =
                    send(client_list.fds[i].fd, nick_msg, strlen(nick_msg), 0);
                if (opt_bytes_sent <= 0)
                  perror("server: nick_msg send");

                broadcast_userlist(&client_list);
                continue;
              }

              char not_found_msg[MAX_MSG_LEN];
              snprintf(not_found_msg, MAX_MSG_LEN, "%s %s\n", ERR_OPT_NOT_FOUND,
                       token);
              send(client_list.fds[i].fd, not_found_msg, strlen(not_found_msg),
                   0);
              continue;
            } else {
              ssize_t bytes_sent;
              char message[MAX_SEND_LEN];
              int msg_len = snprintf(message, MAX_SEND_LEN, "%s: %s",
                                     client_list.clients[i].name, buf);

              for (int j = 1; j < serv_offset; j++) {
                bytes_sent = send(client_list.fds[j].fd, message, msg_len, 0);
                if (bytes_sent <= 0) {
                  perror("server: message send");
                }
              }
            }
          } else if (bytes_received == 0) {
            fprintf(stdout, "client disconnect: %d\n", client_list.fds[i].fd);
            if (i != client_list.count) {
              int closed_fd = client_list.fds[i].fd;
              client_list.fds[i] = client_list.fds[client_list.count];
              client_list.clients[i] = client_list.clients[client_list.count];
              close(closed_fd);
            } else {
              close(client_list.fds[i].fd);
            }
            client_list.count--;
            broadcast_userlist(&client_list);
            continue;
          }
        }

        for (int k = 1; k < serv_offset; k++) {
          if (client_list.fds[k].revents & POLLHUP) {
            int closed_fd = client_list.fds[k].fd;
            if (k != client_list.count) {
              client_list.fds[k] = client_list.fds[client_list.count];
              client_list.clients[k] = client_list.clients[client_list.count];
            }
            close(closed_fd);
            client_list.count--;
            k--;
          }
        }
        if (client_list.count > 0)
          broadcast_userlist(&client_list);
      }
    }
  }

  return -1;
}
