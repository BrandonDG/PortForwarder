/*-----------------------------------------------------------------------
--	SOURCE FILE: header.h
--
--	DATE:		     April 5, 2018
--
--	DESIGNERS:	 Brandon Gillespie & Justen DePourcq
--
--	PROGRAMMERS: Brandon Gillespie & Justen DePourcq
--
--	NOTES:
--	Header file containing shared structures and variables
--  between files used in the 8005 Final Project.
-----------------------------------------------------------------------*/
#define BUFLEN          80
#define TRUE            1
#define HOSTSIZE        10

struct forwarder_connection {
  int client_sd, serverclient_sd;
} forwarder_connection;

struct host_info {
  char ip[30];
  size_t recv_port, send_port;
} host_info;

/*-----------------------------------------------------------------------
--  FUNCTION:   SystemFatal
--
--  DATE:       April 6, 2018
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
