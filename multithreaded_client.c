/*-----------------------------------------------------------------------
--  SOURCE FILE: client.c
--
--  PROGRAM:     multi_clt.exe
--
--  DATE:         February 25, 2018
--
--  DESIGNERS:   Brandon Gillespie & Justen DePourcq
--
--  PROGRAMMERS: Brandon Gillespie
--
--  NOTES:
--  Client for Assignment 2 in COMP8005 BCIT Btech Network
--  Security and Administration
-----------------------------------------------------------------------*/
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <omp.h>

#define SERVER_TCP_PORT   7000
#define BUFLEN            80

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
int main (int argc, char **argv) {
  int n, bytes_to_read, e;
  int sd, port, str_amount;
  struct hostent  *hp;
  struct sockaddr_in server;
  char  *host, *bp, rbuf[BUFLEN], sbuf[BUFLEN], **pptr, lbuf[BUFLEN];
  char str[16];
  struct timeval begin, end;
  size_t data_sent, data_amount;

  data_sent = 0;

  switch(argc) {
    case 2:
      host =  argv[1];
      port =  SERVER_TCP_PORT;
      data_amount = 500000;
    break;
    case 3:
      host =  argv[1];
      data_amount =  atoi(argv[2]);
      port = SERVER_TCP_PORT;
    break;
    default:
      fprintf(stderr, "Usage: %s host [data_amount]\n", argv[0]);
      exit(1);
  }

  bzero((char *)&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if ((hp = gethostbyname(host)) == NULL) {
    fprintf(stderr, "Unknown server address\n");
    exit(1);
  }
  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

  omp_set_num_threads(1000);
  #pragma omp parallel private(sd, rbuf, bytes_to_read, bp, n, lbuf, begin, end, data_sent)
  {
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Cannot create socket");
      exit(1);
    }

    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
      fprintf(stderr, "Can't connect to server\n");
      perror("connect");
      exit(1);
    }
    printf("Connected:    Server Name: %s     Thread: %d\n", hp->h_name, omp_get_thread_num());
    pptr = hp->h_addr_list;
    printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));

    memset(sbuf, 'A', BUFLEN);
    sbuf[BUFLEN - 1] = '\0';

    bp = rbuf;
    bytes_to_read = BUFLEN;
      while (data_sent < data_amount) {

        gettimeofday(&begin, NULL);
        send(sd, sbuf, BUFLEN, 0);

        bp = rbuf;
        bytes_to_read = BUFLEN;
        e = recv(sd, bp, bytes_to_read, MSG_WAITALL);

        gettimeofday(&end, NULL);
        n = end.tv_usec - begin.tv_usec;
        if (n < 0) {
          n *= - 1;
        }
        if (e == -1) break;
        sprintf(lbuf, "%s %d %ld %lu.%06d", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)),
                  sd, sizeof(sbuf), (end.tv_sec - begin.tv_sec), n);
        printf("%s\n", lbuf);
        data_sent += sizeof(sbuf);
      }
      if (e == -1) {
        perror("Connection Error");
      } else {
        sprintf(lbuf, "%s %d %ld %lu.%06d", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)),
                 sd, data_sent, (end.tv_sec - begin.tv_sec), n);
        fflush(stdout);
        close(sd);
      }
    }
}
