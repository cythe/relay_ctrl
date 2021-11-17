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

#define BUF_SIZE 32
#define SOCK_PORT 8090

struct __sockhandler {
    int cfd;
    struct sockaddr_in addr;
    socklen_t addr_len;
};

struct __msg {
    struct list_head list;
    struct __sockhandler sh;
    uint8_t buf[BUF_SIZE];
};

struct __config {
    char tty_name[64];
} config;

LIST_HEAD(msg_list);

int tty_fd = -1;
struct termios oldtio, newtio;
int sfd;
int stop;
pthread_t tid[2];

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

    ret = pthread_join(tid[1], NULL);
    if (ret)
	printf("pthread_join: error\n");

    ret = pthread_join(tid[0], NULL);
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
    int channel;
    int state;
    while(!stop)
    {
	if (list_empty(&msg_list))
	    continue;

	struct __msg * pm = list_entry(msg_list.next, struct __msg, list);
	//printf("dest: pm = %p pm->list = %p\n", pm, &pm->list);
	//printf("...: msg_list = %p, next = %p, pre = %p\n", &msg_list, msg_list.next, msg_list.prev);

	//printf("pm->buf:");
	//printf("cmd = %#x\n", pm->buf[0]);
	//printf("ch = %#x\n", pm->buf[1]);
	//printf("state = %#x\n", pm->buf[2]);

	//write(tty_fd, pm->buf, 3);

	//read(tty_fd, pm->buf, 3);
	usleep(10000);
	if (pm->buf[3] != pm->buf[2] + pm->buf[1])
	{
	    fprintf(stderr, "Error check for msg\n");
	    exit(-1);
	}
	
#if 1
	write(pm->sh.cfd, pm->buf, 4);
#endif

	usleep(10000);
	close(pm->sh.cfd);
	list_del(&pm->list);
	free(pm);
    }
}

void *recv_msg(void *arg)
{
    ssize_t nread;
    struct __sockhandler * ph = arg;
    char host[NI_MAXHOST], service[NI_MAXSERV];
    struct __msg *pm;
    int s;

    printf("recv_msg:\n");
    pm = (struct __msg*)malloc(sizeof(struct __msg));
    memset(pm, 0, sizeof(struct __msg));
    nread = read(ph->cfd, &pm->buf, 4);
    if (nread == -1) {
	printf("Received error!!\n");
	return (void*)(-1);
    }

    s = getnameinfo((struct sockaddr*)&ph->addr, ph->addr_len, 
	    host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
    if (s == 0)
	printf("Received %zd bytes from %s:%s\n",
		nread, host, service);
    else
	fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

    //printf("src: pm = %p pm->list = %p\n", pm, &pm->list);
    printf("buf: ");
    for(int i=0; i<nread; i++)
    {
	printf("%#x ", pm->buf[i]);
    }
    printf("\n");
    memcpy(&pm->sh, ph, sizeof(struct __sockhandler));
    list_add_tail(&pm->list, &msg_list);
    free(arg);

    return NULL;
}

int main(int argc, char *argv[])
{
    int ret;
    char buf[BUF_SIZE];
    struct sockaddr_in addr;

    INIT_LIST_HEAD(&msg_list);
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    parse_ini();
    serial_init(config.tty_name);

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
	perror("socket");
	return -1;
    }

    addr.sin_family = PF_INET;
    addr.sin_port = htons(SOCK_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (ret == -1) {
	perror("bind");
	return -1;
    }

    ret = listen(sfd, 64);
    if (ret == -1) {
	perror("listen");
	return -1;
    }

    ret = pthread_create(&tid[0], NULL, deal_msg, NULL);
    if (ret) {
	perror("pthread_create");
	return -1;
    }

    while (1) {
	struct __sockhandler *psa = (struct __sockhandler*)malloc(sizeof(struct __sockhandler));

	psa->cfd = accept(sfd, (struct sockaddr *)&psa->addr, &psa->addr_len);
	if (psa->cfd == -1) {
	    perror("accept");
	    continue;
	} else {
	    printf("comming\n");
	    ret = pthread_create(&tid[1], NULL, recv_msg, psa);
	    if (ret) {
		perror("pthread_create");
		break;
	    }
	}
    }

    close(sfd);
}
