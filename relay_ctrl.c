#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "iniparser.h"

#define BUF_SIZE 500
#define SOCK_PORT "8090"

#define ON 0
#define OFF 1

int usage(char* me)
{
    printf("usage: %s <1|2|3|4> <on|off>\n", me);
    printf("example: %s 1 on\n", me);
}

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, j;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];
    int channel, state;
    dictionary *ini;

    if (argc < 3) {
	usage(argv[0]);
	return -1;
    }

    channel = atoi(argv[1]);
    if (channel > 4 || channel < 1) {
	usage(argv[0]);
	return -1;
    }
    printf("channel = %d\n", channel);

    if(!strcmp("on", argv[2])) {
	printf("state = ON\n");
	state = ON;
    } else if(!strcmp("off", argv[2])) {
	printf("state = OFF\n");
	state = OFF;
    } else {
	printf("state = UNKNOW\n");
	state = 0xff;
	usage(argv[0]);
	return -1;
    }

    ini = iniparser_load("ctrl.ini");
    if (ini == NULL) {
        fprintf(stderr, "cannot parse file\n");
        return -1 ;
    }
#if 0
    printf("======================\n");
    iniparser_dump(ini, stderr);
    printf("======================\n");
#endif

    printf("RPC:\n");
    const char* server = iniparser_getstring(ini, "RPC:server", "NULL");
    printf("server = [%s]\n", server);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(server, SOCK_PORT, &hints, &result);
    if (s != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
	sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
	if (sfd == -1)
	    continue;
	if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
	    break;                  /* Success */

	close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
	fprintf(stderr, "Could not connect\n");
	exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */

    /* Send remaining command-line arguments as separate
       datagrams, and read responses from server */

    buf[0] = 0x55;
    buf[1] = channel-1;
    buf[2] = state;
    if (write(sfd, buf, 3) != 3) {
	fprintf(stderr, "partial/failed write\n");
	exit(EXIT_FAILURE);
    }

    nread = read(sfd, buf, BUF_SIZE);
    if (nread == -1) {
	perror("read");
	exit(EXIT_FAILURE);
    }

    printf("Received %zd bytes: ", nread);
    for(int i=0;i<nread; i++)
	printf("%#x ", buf[i]);
    printf("\n");

    exit(EXIT_SUCCESS);
}


