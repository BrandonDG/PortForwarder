/*-----------------------------------------------------------------------
--  SOURCE FILE: epoll_forwarder.c
--
--  PROGRAM:     epoll_fwd
--
--  DATE:        April 6, 2018
--
--  DESIGNERS:   Brandon Gillespie & Justen DePourcq
--
--  PROGRAMMERS: Brandon Gillespie
--
--  NOTES:
--  Epoll Forwarder for the Final Project in COMP8005 BCIT BTech Network
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

#include "include/header.h"

#define SERVER_TCP_PORT 7000
#define MAXEVENTS       10000
#define POOLSIZE        6000

/*-----------------------------------------------------------------------
--  FUNCTION:    setup_socket
--
--  DATE:       April 6, 2018
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
--  FUNCTION:   main
--
--  DATE:       April 6, 2018
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
  int server_fd, s, epoll_fd, port, c, c2;
  struct epoll_event event, *events;
  struct host_info info_array[HOSTSIZE];
  char lbuf[BUFLEN], line[BUFLEN], *ip, *portR, *portS, *token;
  FILE* fp;
  struct forwarder_connection fcs[POOLSIZE];

  c = c2 = 0;

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

  if ((fp = fopen("forwarder.conf", "r")) == 0) {
    fprintf(stderr, "configfile fopen\n");
  }

  while (fgets(line, BUFLEN, fp)) {
    token = strtok(line, " \n");
    if(token != NULL) {
        ip = token;
    }

    while(token != NULL) {
        token = strtok(NULL, " \n");
        portR = token;
        token = strtok(NULL, " \n");
        portS = token;
        token = strtok(NULL, " \n");
    }
    strcpy(info_array[c].ip, ip);
    info_array[c].recv_port = atoi(portR);
    info_array[c].send_port = atoi(portS);
    printf("%s %lu %lu\n", info_array[c].ip, info_array[c].recv_port, info_array[c].send_port);
    c++;
  }

  int server_fds[c];

  if ((epoll_fd = epoll_create1(0)) == -1) {
    SystemFatal("epoll_create");
  }

  for (size_t j = 0; j < c; j++) {
    if ((server_fds[j] = setup_socket(info_array[j].recv_port)) == -1) {
      SystemFatal("socket");
    }

    if (fcntl(server_fds[j], F_SETFL, O_NONBLOCK | fcntl(server_fds[j], F_GETFL, 0)) == -1) {
      SystemFatal("fcntl");
    }

    if (listen(server_fds[j], SOMAXCONN) == -1) {
      SystemFatal("listen");
    }

    event.data.fd = server_fds[j];
    event.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
    if ((s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fds[j], &event)) == -1) {
      SystemFatal("epoll_ctl");
    }
  }

  events = calloc(MAXEVENTS, sizeof event);

  while (TRUE) {
    int n, i, b, serverclient_port;
    char *serverclient_ip;

    n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);

    for (i = 0; i < n; i++) {
      b = 0;
      for (size_t j = 0; j < c; j++) {
        if (events[i].data.fd == server_fds[j]) {
          b = 1;
          serverclient_ip = info_array[j].ip;
          serverclient_port = info_array[j].send_port;
        }
      }

      if (events[i].events & (EPOLLHUP | EPOLLERR)) {
        close (events[i].data.fd);
        continue;
      } else if (b) {
          while (TRUE) {
            struct sockaddr_in remote_addr, serverclient;
            socklen_t addr_size;
            int new_sd, serverclient_sd;
            struct hostent *hp;

            addr_size = sizeof(remote_addr);
            new_sd = accept(events[i].data.fd, (struct sockaddr*) &remote_addr, &addr_size);
            if (new_sd == -1) {
              if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept");
              }
              break;
            }

            // Resolve serverlcient host
            bzero((char *)&serverclient, sizeof(struct sockaddr_in));
            serverclient.sin_family = AF_INET;
            serverclient.sin_port = htons(serverclient_port);
            if ((hp = gethostbyname(serverclient_ip)) == NULL) {
              fprintf(stderr, "Unknown server address\n");
              exit(1);
            }
            bcopy(hp->h_addr, (char *)&serverclient.sin_addr, hp->h_length);

            // Serverclient socket
            if ((serverclient_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
              perror("Cannot create socket");
              exit(1);
            }
            // Connect to server client
            if (connect(serverclient_sd, (struct sockaddr *)&serverclient, sizeof(serverclient)) == -1) {
              fprintf(stderr, "Can't connect to server\n");
              perror("connect");
              exit(1);
            }

            if (fcntl(new_sd, F_SETFL, O_NONBLOCK | fcntl (events[i].data.fd, F_GETFL, 0)) == -1) {
              SystemFatal("fcntl");
            }
            // Make serverclient socket non blocking
            if (fcntl(serverclient_sd, F_SETFL, O_NONBLOCK) == -1) {
              SystemFatal("fcntl");
            }

            event.data.fd = new_sd;
            event.events = EPOLLIN | EPOLLET;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sd, &event) == -1) {
              SystemFatal("epoll_ctl");
            }
            // Add serverclient socket to epoll
            event.data.fd = serverclient_sd;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverclient_sd, &event) == -1) {
              SystemFatal("epoll_ctl");
            }
            printf(" Remote Address:  %s\n", inet_ntoa(remote_addr.sin_addr));
            if (c2 >= POOLSIZE) { c2 = 0; }
            fcs[c2].serverclient_sd = serverclient_sd;
            fcs[c2].client_sd = new_sd;
            c2++;
          }
            continue;
          } else {
            int f, dst_sd;
            ssize_t n, data_sent;
            f = data_sent = 0;
            while (TRUE) {
              char buf[BUFLEN];

              for (size_t j = 0; j < POOLSIZE; j++) {
                if (events[i].data.fd == fcs[j].serverclient_sd) {
                  dst_sd = fcs[j].client_sd;
                  break;
                }
                if (events[i].data.fd == fcs[j].client_sd) {
                  dst_sd = fcs[j].serverclient_sd;
                  break;
                }
              }

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

              s = write(dst_sd, buf, n);
              if (s == -1) {
                perror("write");
                SystemFatal("write");
              }
            }

            sprintf(lbuf, "Receiving Socket: %u   Sending Socket: %u", events[i].data.fd, dst_sd);
            printf("%s\n", lbuf);

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
