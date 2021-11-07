#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#define ON 0
#define OFF 1

#define TTY "/dev/ttyUSB0"

int usage(char* me)
{
    printf("usage: %s <channel> <on|off>\n", me);
    printf("example: %s 1 on\n", me);
}

int main(int argc, char* argv[])
{
    int tty_fd;
    int channel;
    int state;
    uint8_t buf[3] = {0};
    struct termios oldtio, newtio;

    if (argc < 3) {
	usage(argv[0]);
	return -1;
    }

    tty_fd = open(TTY, O_RDWR|O_NOCTTY);
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

    channel = atoi(argv[1]);
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
    }

    buf[0] = 0x55;
    buf[1] = channel;
    buf[2] = state;

    write(tty_fd, buf, 3);

    tcsetattr(tty_fd, TCSANOW, &oldtio);
    tcflush(tty_fd, TCIOFLUSH);


    close(tty_fd);
    return 0;
}
