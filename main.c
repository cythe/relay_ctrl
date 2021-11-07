#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#include "iniparser.h"

#define ON 0
#define OFF 1

//#define TTY "/dev/ttyUSB0"

int usage(char* me)
{
    printf("usage: %s <1|2|3|4> <on|off>\n", me);
    printf("example: %s 1 on\n", me);
}

int main(int argc, char* argv[])
{
    int ret;
    int tty_fd;
    int channel;
    int state;
    uint8_t buf[3] = {0};
    struct termios oldtio, newtio;
    dictionary  *   ini ;
    const char  *tty_name;

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

    ini = iniparser_load("conf.ini");
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
    tty_name = iniparser_getstring(ini, "tty:file", "NULL");
    printf("tty_name = [%s]\n", tty_name);

    tty_fd = open(tty_name, O_RDWR|O_NOCTTY);
    if (tty_fd < 0) {
	printf("error@%s(%d): Open tty device file.\n", __func__, __LINE__);
	ret = -1;
	goto error;
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

    buf[0] = 0x55;
    buf[1] = channel-1;
    buf[2] = state;

    write(tty_fd, buf, 3);

    tcsetattr(tty_fd, TCSANOW, &oldtio);
    tcflush(tty_fd, TCIOFLUSH);

    close(tty_fd);

error:
    iniparser_freedict(ini);
    return ret;
}
