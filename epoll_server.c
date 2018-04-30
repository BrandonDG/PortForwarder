/*-----------------------------------------------------------------------
--  SOURCE FILE: epoll_server.c
--
--  PROGRAM:     epoll_svr.exe
--
--  DATE:        February 25, 2018
--
--  DESIGNERS:   Brandon Gillespie & Justen DePourcq
--
--  PROGRAMMERS: Brandon Gillespie
--
--  NOTES:
--  Epoll Server for Assignment 2 in COMP8005 BCIT Btech Network
--  Security and Administration
-----------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define SERVER_TCP_PORT 7000
#define TRUE            1
#define BUFLEN          80
#define MAXEVENTS       10000

/*-----------------------------------------------------------------------
--  FUNCTION:   setup_socket
--
--  DATE:       February 28, 2018
--
--  INTERFACE:  static int setup_socket(int port)
--
--  RETURNS:    int
--
--  NOTES:
--  Create and binds the server socket.
-----------------------------------------------------------------------*/
static int setup_socket(int port) {
  int fd_server, arg;
  struct sockaddr_in addr;

  if ((fd_server = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    return(-1);
  }

  arg = 1;
  if (setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) {
    return(-1);
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(fd_server, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
    return -1;
  }
  return(fd_server);
}

/*-----------------------------------------------------------------------
--  FUNCTION:    SystemFatal
--
--  DATE:       February 25, 2018
--
--  INTERFACE:  static void SystemFatal(const char* message)
--
--  RETURNS:    void
--
--  NOTES:
--  Displays error message in perror and exits the application.
-----------------------------------------------------------------------*/
static void SystemFatal(const char* message) {
  perror(message);
  exit(EXIT_FAILURE);
}

/*-----------------------------------------------------------------------
--  FUNCTION:    main
--
--  DATE:       February 25, 2018
--
--  DESIGNER:   Brandon Gillespie & Justen DePourcq
--
--  PROGRAMMER:  Brandon Gillespie
--
--  INTERFACE:  int main (int argc, char **argv)
--
--  RETURNS:    int
--
--  NOTES:
--  main.
-----------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
  int server_fd, s, epoll_fd, port;
  struct epoll_event event, *events;
  FILE *fp;
  char lbuf[BUFLEN];

  switch (argc) {
    case 1:
      port = SERVER_TCP_PORT;
    break;
    case 2:
      port = atoi(argv[1]);
    break;
    default:
      fprintf(stderr, "[command] [port]\n");
      exit(1);
    break;
  }

  if ((server_fd = setup_socket(port)) == -1) {
    SystemFatal("socket");
  }

  if (fcntl(server_fd, F_SETFL, O_NONBLOCK | fcntl (server_fd, F_GETFL, 0)) == -1) {
    SystemFatal("fcntl");
  }

  if (listen(server_fd, SOMAXCONN) == -1) {
    SystemFatal("listen");
  }

  if ((epoll_fd = epoll_create1(0)) == -1) {
    SystemFatal("epoll_create");
  }

  event.data.fd = server_fd;
  event.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
  if ((s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event)) == -1) {
    SystemFatal("epoll_ctl");
  }

  events = calloc(MAXEVENTS, sizeof event);

  if ((fp = fopen("epoll_server_results", "w")) == 0) {
    fprintf(stderr, "server fopen\n");
    exit(1);
  }
  fclose(fp);

  if ((fp = fopen("epoll_server_results", "a")) == 0) {
    fprintf(stderr, "server fopen\n");
    exit(1);
  }

  while (TRUE) {
    int n, i;

    n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
    for (i = 0; i < n; i++) {
      if (events[i].events & (EPOLLHUP | EPOLLERR)) {
        close (events[i].data.fd);
        continue;
      } else if (server_fd == events[i].data.fd) {
          while (TRUE) {
            struct sockaddr_in remote_addr;
            socklen_t addr_size;
            int new_sd;

            addr_size = sizeof(remote_addr);
            new_sd = accept(server_fd, (struct sockaddr*) &remote_addr, &addr_size);
            if (new_sd == -1) {
              if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept");
              }
              break;
            }

            if (fcntl(new_sd, F_SETFL, O_NONBLOCK | fcntl (server_fd, F_GETFL, 0)) == -1) {
              SystemFatal("fcntl");
            }

            event.data.fd = new_sd;
            event.events = EPOLLIN | EPOLLET;
              if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sd, &event) == -1) {
                SystemFatal("epoll_ctl");
              }
              printf(" Remote Address:  %s\n", inet_ntoa(remote_addr.sin_addr));
            }
            continue;
          } else {
            int f;
            ssize_t n, data_sent;
            f = data_sent = 0;
            while (TRUE) {
              char buf[BUFLEN];

              n = read(events[i].data.fd, buf, sizeof buf);
              if (n == -1) {
                if (errno != EAGAIN) {
                  perror("read");
                  f = 1;
                }
                break;
              } else if (n == 0) {
                f = 1;
                break;
              }
              data_sent += sizeof(buf);

              s = write(events[i].data.fd, buf, n);
              if (s == -1) {
                perror("write");
                SystemFatal("write");
              }
            }

            sprintf(lbuf, "Socket: %u   Amount of Data: %ld", events[i].data.fd, data_sent);
            printf("%s\n", lbuf);
            fprintf(fp, "%s\n", lbuf);

            if (f) {
              close(events[i].data.fd);
            }
          }
    }
  }

  free(events);

  close(server_fd);
  fclose(fp);
  return 0;
}
