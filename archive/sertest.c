//#include <bcm2835.h>
#include <stdio.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int opentty()
{
/*	int fd;

	fd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);
	if(fd == -1)
	{
		perror("opentty: cannot open /dev/ttyAMA0 - \n");
	}
	else
	{
		printf("opened /dev/ttyAMA0\n");
		fcntl(fd, F_SETFL, 0);
	}
	close(fd);
	return fd; */
}

int main()
{
	//opentty();

	int fd;
	char buf[255];
	char *bufptr;
	bufptr = buf;
	int bytenum;

	struct termios options;

//	bcm2835_init();


	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	if(fd == -1)
	{
		perror("could not open /dev/ttyAMA0 - ");
	}
	else
	{
		fcntl(fd, F_SETFL, 0);
	}

	tcgetattr(fd, &options);

	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);

/*	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	options.c_lflag |= (ICANON | ECHO | ECHOE);*/

	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	options.c_cflag &= ~CRTSCTS;
	options.c_cc[VMIN] = 1;
	options.c_cc[VTIME] = 5;
	options.c_cflag |= CREAD | CLOCAL;

//	int count;
  //              for(count = 0; count < 100; count++)
    //            {
      //                  printf("asdf ");
        //        }


	printf("now entering while loop\n");

	write(fd, "Ready\n\r", 7);

	memset(buf, 0, sizeof(buf));


	//while((bytenum = read(fd, bufptr, buf + sizeof(buf) - bufptr - 1)) > 0)

		//printf("read fd into bytenum\n");
		//bytenum = 
	while(1)
	{
		printf("-----START READ-----\n");
		if( read(fd, bufptr, buf + sizeof(buf) - bufptr - 1) == -1)
		{
			printf("read() last errno: - %s\n",strerror(errno));
			printf("errno raw value: - %d\n", errno);
			goto abort;
		}

		bufptr = buf;

		printf("got data: %s\n", buf);

		int i = strlen(buf);
		int count;

		printf("data length: %d\n", i);

		/*printf("%d ", buf[0]);
		printf("%d ", buf[1]);
                printf("%d ", buf[2]);
                printf("%d ", buf[3]);*/

		for(count = 0; count < i; count++)
		{
			printf("%d ", buf[count]);
		}
		printf("\n");

		if(buf[0] == 102)
		{
			printf("got cmd f\n");
			write(fd, "OK\r\n", 4);
		}
		else
		{
			printf("unknown cmd: %s\n", buf);
		}

		memset(buf, 0, sizeof(buf));

		if(1 == 1)
		{

		}
		printf("-----END   READ-----\n");

	}

abort:
	printf("closing port and terminating\n");
	close(fd);


	return 0;
}
