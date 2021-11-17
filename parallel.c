#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define BUF_SIZE 500
#define SOCK_PORT "8090"
#define SERVER "pek-rhao-d1.corp.ad.wrs.com"

#define ON 0
#define OFF 1

volatile int stop;
pthread_t tid[16];

void * p_rand(void *arg)
{
    pthread_t *pt = arg;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, j;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];
    int channel, state;

    printf("create %ld\n", *pt);
    srand(time(NULL));

    while(!stop)
    {
	usleep(random()%10000);

	channel = random() % 20; 

	usleep(random()%10000);

	state = random() % 2; 
	printf("*pt[%ld]: Rq: ch = %#x, st = %#x\n", *pt, channel, state);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo(SERVER, SOCK_PORT, &hints, &result);
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
	buf[1] = channel;
	buf[2] = state;
	buf[3] = channel + state;
	if (write(sfd, buf, 4) != 4) {
	    fprintf(stderr, "partial/failed write\n");
	    exit(EXIT_FAILURE);
	}
#if 1
	memset(buf, 0, 4);

	nread = read(sfd, buf, BUF_SIZE);
	if (nread == -1) {
	    perror("read");
	    exit(EXIT_FAILURE);
	}

	printf("*pt[%ld]: Rs: ch = %#x, st = %#x\n", *pt, buf[1], buf[2]);
	if (buf[3]!= buf[1]+buf[2]) {
	    printf("error !!!!!!!!!!!!!!!!!!!!!!! \n");
	    printf("[%ld]re: %d %d rs: %#x %#x %#x %#x\n", *pt, channel, state, buf[0], buf[1], buf[2], buf[3]);
	    exit(EXIT_FAILURE);
	}
	if (buf[1] != channel || buf[2] != state) {
	    printf("error !!!!!!!!!!!!!!!!!!!!!!! \n");
	    printf("[%ld]re: %d %d rs: %#x %#x %#x %#x\n", *pt, channel, state, buf[0], buf[1], buf[2], buf[3]);
	    exit(EXIT_FAILURE);
	}
#endif
	close(sfd);
    }
}
 
void sig_handler(int signum)
{
    int ret;
    int i;
    stop = 1;
    for(i = 0; i < 16; i++)
    {
	ret = pthread_join(tid[i], NULL);
	if (ret)
	    printf("pthread_join: [%ld] error\n", tid[i]);
    }

    printf("Bye-bye.\n");
}

int main(int argc, char *argv[])
{
    int i;
    stop = 0;
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    for(i = 0; i < 16; i++)
    {
	pthread_create(&tid[i], NULL, p_rand, &tid[i]);
	pthread_detach(tid[i]);
    }

    while(1)
	sleep(0);

    return 0;
}


