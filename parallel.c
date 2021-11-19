#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define BUF_SIZE 500
#define SOCK_PORT 8090
#define SERVER "pek-rhao-d1.corp.ad.wrs.com"

#define ON 0
#define OFF 1

volatile int stop;
pthread_t tid[16];

void * p_rand(void *arg)
{
    pthread_t *pt = arg;
    int sfd, s, j;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];
    int channel, state;
    struct sockaddr_in dest;

    printf("create %ld\n", *pt);
    srand(time(NULL));

    while(!stop)
    {
	usleep(random()%10000);

	channel = random() % 20; 

	usleep(random()%10000);

	state = random() % 2; 
	printf("*pt[%ld]: Rq: ch = %#x, st = %#x\n", *pt, channel, state);

	sfd = socket(AF_INET, SOCK_STREAM, 0);

	dest.sin_family = AF_INET;
	dest.sin_port = htons(SOCK_PORT);
	dest.sin_addr.s_addr=inet_addr("127.0.0.1"); //128.224.162.151");

	connect(sfd, (struct sockaddr *)&dest, sizeof(struct sockaddr));

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

    pthread_detach(pthread_self());
}
 
void sig_handler(int signum)
{
    int ret;
    int i;
    stop = 1;
#if 0
    void* res;
    for(i = 0; i < 16; i++)
    {
	ret = pthread_join(tid[i], &res);
	if (ret)
	    perror("pthread_join");
    }
#endif

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
    }

    while(1)
	sleep(1);

    return 0;
}


