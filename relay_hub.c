#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>

#include "list.h"
#include "iniparser.h"

#define BUF_SIZE 500
#define SOCK_PORT "8090"

struct __msg {
    struct list_head list;
    uint8_t buf[32];
};

struct __config {
    char tty_name[64];
} config;

LIST_HEAD(msg_list);

int tty_fd = -1;
struct termios oldtio, newtio;
int sfd;
int stop;
pthread_t tid;
struct sockaddr_storage peer_addr;
socklen_t peer_addr_len;

int parse_ini(void)
{
    dictionary *ini;

    ini = iniparser_load("hub.ini");
    if (ini == NULL) {
        fprintf(stderr, "cannot parse file\n");
        return -1 ;
    }
#if 0
    printf("======================\n");
    iniparser_dump(ini, stderr);
    printf("======================\n");
#endif

    printf("tty:\n");
    strncpy(config.tty_name, iniparser_getstring(ini, "tty:file", "NULL"), 64);
    printf("tty_name = [%s]\n", config.tty_name);

    return 0;
}

int serial_init(char* tty_name)
{
    printf("open %s\n", tty_name);
    return 0;
    tty_fd = open(tty_name, O_RDWR|O_NOCTTY);
    if (tty_fd < 0) {
	printf("error@%s(%d): Open tty device file.\n", __func__, __LINE__);
	return -1;
    }

    tcgetattr(tty_fd, &oldtio);
    tcgetattr(tty_fd, &newtio);

    newtio.c_cflag |= CLOCAL;
    newtio.c_cflag |= CREAD;
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag &= ~CRTSCTS;

    newtio.c_iflag &= IXOFF;
    newtio.c_iflag &= IXON;
    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    newtio.c_oflag &= ~OPOST;
    //newtio.c_cc[VTIME] = 0;
    //newtio.c_cc[VMIN] = 3;

    cfsetispeed(&newtio, B115200);
    cfsetospeed(&newtio, B115200);

    newtio.c_cflag |= CS8;

    newtio.c_cflag &= ~CSTOPB;

    tcsetattr(tty_fd, TCSANOW, &newtio);
    tcflush(tty_fd, TCIOFLUSH);

    return 0;
}

void serial_deinit(void)
{
    printf("Close the tty port\n");
    tcsetattr(tty_fd, TCSANOW, &oldtio);
    tcflush(tty_fd, TCIOFLUSH);
    close(tty_fd);
}

void sig_handler(int signum)
{
    int ret;
    serial_deinit();
    stop = 1;
    ret = pthread_join(tid, NULL);
    if (ret)
	printf("pthread_join: error\n");

    printf("Bye-bye.\n");
}

int sock_init(void)
{
    return 0;
}

void *deal_msg(void *arg)
{
    uint8_t buf[3] = {0};
    int channel;
    int state;
    while(!stop)
    {
	memset(buf, 0, 3);

	if (list_empty(&msg_list))
	    continue;

	struct __msg * pm = list_entry(msg_list.next, struct __msg, list);
	//printf("dest: pm = %p pm->list = %p\n", pm, &pm->list);
	//printf("...: msg_list = %p, next = %p, pre = %p\n", &msg_list, msg_list.next, msg_list.prev);

	//printf("pm->buf:");
	//printf("cmd = %#x\n", pm->buf[0]);
	//printf("ch = %#x\n", pm->buf[1]);
	//printf("state = %#x\n", pm->buf[2]);

	write(tty_fd, pm->buf, 3);

	read(tty_fd, pm->buf, 3);
	if (sendto(sfd, pm->buf, 3, 0,
		    (struct sockaddr *) &peer_addr,
		    peer_addr_len) != 3)
	    fprintf(stderr, "Error sending response\n");

	list_del(&pm->list);
	free(pm);
    }
}

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;
    int ret;
    ssize_t nread;
    char buf[BUF_SIZE];

    INIT_LIST_HEAD(&msg_list);
    signal(SIGTERM, sig_handler);
    parse_ini();
    serial_init(config.tty_name);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, SOCK_PORT, &hints, &result);
    if (s != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
	sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
	if (sfd == -1)
	    continue;

	if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
	    break;                  /* Success */

	close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
	fprintf(stderr, "Could not bind\n");
	exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */

    ret = pthread_create(&tid, NULL, deal_msg, NULL);
    if (ret) {
	perror("pthread_create");
	return -1;
    }

    /* Read datagrams and echo them back to sender */

    while (1) {
	peer_addr_len = sizeof(struct sockaddr_storage);
	nread = recvfrom(sfd, buf, BUF_SIZE, 0,
		(struct sockaddr *) &peer_addr, &peer_addr_len);
	if (nread == -1)
	    continue;               /* Ignore failed request */

	char host[NI_MAXHOST], service[NI_MAXSERV];

	s = getnameinfo((struct sockaddr *) &peer_addr,
		peer_addr_len, host, NI_MAXHOST,
		service, NI_MAXSERV, NI_NUMERICSERV);
	if (s == 0)
	    printf("Received %zd bytes from %s:%s\n",
		    nread, host, service);
	else
	    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

	struct __msg *pm = (struct __msg*)malloc(sizeof(struct __msg));
	//printf("src: pm = %p pm->list = %p\n", pm, &pm->list);
	memset(pm, 0, sizeof(struct __msg));
	printf("buf: ");
	for(int i=0; i<nread; i++)
	{
	    printf("%#x ", buf[i]);
	    pm->buf[i] = buf[i];
	}
	printf("\n");
	list_add(&pm->list, &msg_list);
    }
}
