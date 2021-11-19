#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "iniparser.h"

#define BUF_SIZE 500
#define SOCK_PORT 8090

#define ON 0
#define OFF 1

int usage(char* me)
{
    printf("usage: %s <channel> <on|off>\n", me);
    printf("example: %s 1 on\n", me);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in dest;
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
    //if (channel > 4 || channel < 1) {
    //    usage(argv[0]);
    //    return -1;
    //}
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

    sfd = socket(AF_INET, SOCK_STREAM, 0);

    dest.sin_family = AF_INET;
    dest.sin_port = htons(SOCK_PORT);
    dest.sin_addr.s_addr=inet_addr("128.224.163.137"); //128.224.162.151");

    connect(sfd, (struct sockaddr *)&dest, sizeof(struct sockaddr));

    buf[0] = 0x55;
    buf[1] = channel-1;
    buf[2] = state;
    buf[3] = buf[1]+buf[2];

    if (write(sfd, buf, 4) != 4) {
	fprintf(stderr, "partial/failed write\n");
	exit(EXIT_FAILURE);
    }
#if 1
    nread = read(sfd, buf, BUF_SIZE);
    if (nread == -1) {
	perror("read");
	exit(EXIT_FAILURE);
    }

    printf("Received %zd bytes: ", nread);
    for(int i=0;i<nread; i++)
	printf("%#x ", buf[i]);
    printf("\n");
#endif

    exit(EXIT_SUCCESS);
}


