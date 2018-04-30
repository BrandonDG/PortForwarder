/*-----------------------------------------------------------------------
--	SOURCE FILE: multhithreaded_server.c
--
--	PROGRAM:     multi_fwd
--
--	DATE:		     April 5, 2018
--
--	DESIGNERS:	 Brandon Gillespie & Justen DePourcq
--
--	PROGRAMMERS: Brandon Gillespie
--
--	NOTES:
--	Multithreaded Forwarder for Final Project in COMP8005 BCIT Btech
--  Network Security and Administration
-----------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <omp.h>
#include <errno.h>
#include <pthread.h>

#include "include/header.h"

#define SERVER_TCP_PORT 7000

/*-----------------------------------------------------------------------
--	FUNCTION:	  main
--
--	DATE:       April 5, 2018
--
--	DESIGNER:   Brandon Gillespie & Justen DePourcq
--
--  PROGRAMMER:	Brandon Gillespie
--
--	INTERFACE:	int main (int argc, char **argv)
--
--	RETURNS:    int
--
--	NOTES:
--	main.
-----------------------------------------------------------------------*/
int main(int argc, char **argv) {
  int sd, new_sd, data_sent, e, client_len;
  struct sockaddr_in server, client, serverclient;
  struct hostent *hp;
  int thread_num, bytes_to_read;
  char lbuf[BUFLEN], buf[BUFLEN], *bp;
  char line[BUFLEN], *ip, *portR, *token, *portS;
  FILE* fp;
  struct host_info info_array[HOSTSIZE];
  int i = 0;

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
    strcpy(info_array[i].ip, ip);
    info_array[i].recv_port = atoi(portR);
    info_array[i].send_port = atoi(portS);
    i++;
  }

  omp_set_nested(1);
  omp_set_num_threads(50);
  #pragma omp parallel for private(sd, new_sd, data_sent, e, client_len, server, serverclient, client, thread_num, bytes_to_read, lbuf, buf, bp)
  for (size_t j = 0; j < i; j++) {

    printf("%s %lu %lu\n", info_array[j].ip, info_array[j].recv_port, info_array[j].send_port);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Failed to create socket");
    }

    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(info_array[j].recv_port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
      perror("Failed to bind socket");
      exit(1);
    }

    int serverclient_sd;

    listen(sd, 4096);
    #pragma omp parallel private(thread_num, new_sd, lbuf, client, serverclient_sd, serverclient, data_sent)
    {
      while (TRUE) {
        data_sent = 0;
        client_len = sizeof(client);
        if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
          fprintf(stderr, "Can't accept client\n");
          exit(1);
        }

        bzero((char *)&serverclient, sizeof(struct sockaddr_in));
        serverclient.sin_family = AF_INET;
        serverclient.sin_port = htons(info_array[j].send_port);
        if ((hp = gethostbyname(info_array[j].ip)) == NULL) {
          fprintf(stderr, "Unknown server address\n");
          exit(1);
        }
        bcopy(hp->h_addr, (char *)&serverclient.sin_addr, hp->h_length);

        if ((serverclient_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
          perror("Cannot create socket");
          exit(1);
        }

        if (connect(serverclient_sd, (struct sockaddr *)&serverclient, sizeof(server)) == -1) {
          fprintf(stderr, "Can't connect to server\n");
          perror("connect");
          exit(1);
        }

        thread_num = omp_get_thread_num() + 1;
        bp = buf;
        bytes_to_read = BUFLEN;
        while ((e = recv(new_sd, bp, bytes_to_read, MSG_WAITALL)) > 0) {
          // Send client message to server
          send(serverclient_sd, buf, BUFLEN, 0);
          // Receive server message
          recv(serverclient_sd, bp, bytes_to_read, MSG_WAITALL);
          // Send server response to client
          send(new_sd, buf, BUFLEN, 0);
          sprintf(lbuf, "%d %s %u %ld", sd, inet_ntoa(client.sin_addr), thread_num, sizeof(buf));
          printf("%s\n", lbuf);

        }

        if (e == -1) {
          perror("Connection Error");
        } else {
          sprintf(lbuf, "%s %u %ld", inet_ntoa(client.sin_addr), thread_num, data_sent);
        }

        close(new_sd);
        close(serverclient_sd);
        fclose(fp);
      }
    }
  }

  return 0;
}
