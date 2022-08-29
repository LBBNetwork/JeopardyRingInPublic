/* Code for Jeopardy Ring-in Device, Mk.IV

   Hardware configuration:
   * Raspberry Pi - Any model, tested with Model B, 3 B+, 4 4GB
   * Arduino-based ringin/countdown controllers
   * TTL to RS-232 adapter on the Pi's TTL pins

   Software configuration:
   * Requires the bcm2835 library for GPIO. Get it from:
     http://www.airspayce.com/mikem/bcm2835/
   * TTL pins must be active in raspi-config

   Compiling:
   * Ensure you have gcc and the bcm2835 library installed.
     To compile, simply issue this command:

     make

   Running:
   * Since this program uses GPIO pins, you must run the
     program as root.

   Have fun!
*/

/* Filename: gpio.c
   Author: neko2k (neko2k@beige-box)
   Website: http://www.beige-box.com
   Description: See above

   The following source code is (c) 2014-2021 The Little Beige Box
   and is released as open-source software under the terms of the
   Pirate License. Yar har fiddle dee dee being a pirate is alright
   to be do what you want cause a pirate is free you are a pirate!

   support@beige-box.com
*/

#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

#define ROOT_UID 0		// Change this define to match your root UID if CheckIfRoot() says you have a root UID mismatch.
#define MODEL_BPLUS		// Define this as MODEL_AB or MODEL_BPLUS depending on your Pi model.

#define MS_DIVISOR 10		// Play with this and see if this makes a difference to InterruptDelay() precision.

#ifdef PI_MODEL
	#error Must define a Pi type as MODEL_AB (26-pin) or MODEL_BPLUS (40-pin)
#endif
#ifdef MODEL_AB //map the pin assignments to the 26-pin Model A/B Pi
	#define INPUT1 		RPI_GPIO_P1_07			//Pin 4
	#define INPUT2 		RPI_GPIO_P1_11			//Pin 17
	#define INPUT3 		RPI_V2_GPIO_P1_13		//Pin 27 (Rev2)
	//#define INPUT3 	RPI_GPIO_P1_13			//Pin 21 (Rev1)
	#define INPUT4 		RPI_GPIO_P1_15			//Pin 22
	#define P1_LED 		RPI_GPIO_P1_12			//Pin 18
	#define P2_LED 		RPI_GPIO_P1_16			//Pin 23
	#define P3_LED 		RPI_GPIO_P1_18			//Pin 24
//	#define P4_LED 		RPI_GPIO_P1_22			//Pin 25
	#define LOCKOUT_ASSERT	RPI_GPIO_P1_11			//Pin 11 (SCLK)
#endif
#ifdef MODEL_BPLUS //map the pin assignments to the 40-pin Model B+ (and all later revisions)
	#define INPUT1		RPI_BPLUS_GPIO_J8_11		//Pin 11 (GPIO 17)
	#define INPUT2		RPI_BPLUS_GPIO_J8_13		//Pin 13 (GPIO 27)
	#define INPUT3		RPI_BPLUS_GPIO_J8_15		//Pin 15 (GPIO 22)
//	#define INPUT4		RPI_BPLUS_GPIO_J8_7		//Pin 7  (GPIO 4)
	#define P1_LED		RPI_BPLUS_GPIO_J8_29		//Pin 29 (GPIO 5)
	#define P2_LED		RPI_BPLUS_GPIO_J8_31		//Pin 31 (GPIO 6)
	#define P3_LED		RPI_BPLUS_GPIO_J8_33		//Pin 33 (GPIO 13)
//	#define P4_LED		RPI_BPLUS_GPIO_J8_37		//Pin 37 (GPIO 26)
	#define LOCKOUT_ASSERT	RPI_BPLUS_GPIO_J8_32		//Pin 32 (GPIO 12)

	//Define the countdown timer lights
	#define P1_ENABLE	RPI_BPLUS_GPIO_J8_07		//Pin 7  (GPIO 4)
	#define P2_ENABLE	RPI_BPLUS_GPIO_J8_05		//Pin 5  (GPIO 3)
	#define P3_ENABLE	RPI_BPLUS_GPIO_J8_03		//Pin 3  (GPIO 2)
	#define TIME_1		RPI_BPLUS_GPIO_J8_19		//Pin 19 (GPIO 10 / MOSI)
	#define TIME_2		RPI_BPLUS_GPIO_J8_23		//Pin 23 (GPIO 11 / CLK)
	#define TIME_3		RPI_BPLUS_GPIO_J8_21		//Pin 21 (GPIO 9 / MISO)
	#define TIME_4		RPI_BPLUS_GPIO_J8_35		//Pin 35 (GPIO 19)
	#define TIME_5		RPI_BPLUS_GPIO_J8_37		//Pin 37 (GPIO 26)
#endif


//Used in Mk.II. Pin assignments changed in Mk.III.
/*#define INPUT1 RPI_GPIO_P1_16                   //Pin 23
#define INPUT2 RPI_GPIO_P1_11                   //Pin 17
#define INPUT3 RPI_GPIO_P1_18                   //Pin 24
#define ENABLER RPI_GPIO_P1_12                  //Pin 18
#define ENABLER_LED RPI_GPIO_P1_07              //Pin 4
#define P1_LED RPI_GPIO_P1_15                   //Pin 22
#define P2_LED RPI_GPIO_P1_22                   //Pin 25
#define P3_LED RPI_V2_GPIO_P1_13                //Pin 27 (Rev2) */
/*#define P3_LED RPI_GPIO_P1_13*/               //Pin 21 (Rev1)
//#define OPERATOR_INTERRUPT RPI_V2_GPIO_P1_05    //Pin SCL (GPIO 3, Rev2)
/*#define OPERATOR_INTERRUPT RPI_GPIO_P1_05*/   //Pin SCL? (GPIO 1, Rev1)

typedef struct SerData {
        int StatusByte;
} SerData;

int GetPlayerRingin(int PlayerInput, RPiGPIOPin playerLED);
int TTLOpen();
int TTLClose();
int TTLRead();
int TTLWrite();

void ClearCountdownLights(int player);

void *SerialThread(void *thread);

void InterruptDelay(int milliseconds, bool selftest, SerData dataread);
void CheckIfRoot();
void CleanupAndClose();

int main()
{
	/* Hook ^C */
	signal(SIGINT, CleanupAndClose);

	/* Clear screen and display startup text */
	printf("\033[H\033[J");
	printf("main(): Jeopardy Ring-In Device Mk. V\nCopyright (c) 2014-2022 The Little Beige Box\nwww.beige-box.com\n\nSELF TEST START\n\n");

#ifdef MODEL_AB
	printf("main(): Compiled for 26-pin Model A/B. Countdown timer is disabled.\n");
#endif
#ifdef MODEL_BPLUS
	printf("main(): Compiled for 40-pin Model B+. Countdown timer is enabled.\n");
#endif

        uint8_t lockout, value1, value2, value3;
        int P1Lockout, P2Lockout, P3Lockout;

	pthread_t ser;
	SerData *DataReadPtr = malloc(sizeof(SerData));
	SerData DataRead;

	/* Check if current UID is the ROOT_UID */
	CheckIfRoot();

        if(!bcm2835_init())
                return 1;

	InterruptDelay(750, true, DataRead);

        /* Set up the GPIO pins for input */
	printf("main(): Setting up GPIO input... ");

	bcm2835_gpio_fsel(INPUT1, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(INPUT1, BCM2835_GPIO_PUD_UP);
	printf("INPUT1 ");

        bcm2835_gpio_fsel(INPUT2, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(INPUT2, BCM2835_GPIO_PUD_UP);
	printf("INPUT2 ");

        bcm2835_gpio_fsel(INPUT3, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(INPUT3, BCM2835_GPIO_PUD_UP);
	printf("INPUT3 ");

	printf("- OK\n");

        /* Set up the GPIO pins for LED output &
           perform a self-test of all LEDs */
	printf("main(): Setting up GPIO Outputs... ");

	printf("P1_LED ");
        bcm2835_gpio_fsel(P1_LED, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(P1_LED, HIGH);
	InterruptDelay(750, true, DataRead);
	bcm2835_gpio_write(P1_LED, LOW);

	printf("P2_LED ");
	bcm2835_gpio_fsel(P2_LED, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(P2_LED, HIGH);
	InterruptDelay(750, true, DataRead);
	bcm2835_gpio_write(P2_LED, LOW);

	printf("P3_LED ");
        bcm2835_gpio_fsel(P3_LED, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(P3_LED, HIGH);
	InterruptDelay(750, true, DataRead);
	bcm2835_gpio_write(P3_LED, LOW);

	printf("LOCKOUT_ASSERT ");
	bcm2835_gpio_fsel(LOCKOUT_ASSERT, BCM2835_GPIO_FSEL_OUTP);
	InterruptDelay(750, true, DataRead);

	printf("- OK\n");

	printf("main(): Start test of player countdowns timers... ");

	printf("TIME_X ");
	bcm2835_gpio_fsel(TIME_1, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(TIME_2, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(TIME_3, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(TIME_4, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(TIME_5, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(TIME_1, HIGH);
	bcm2835_gpio_write(TIME_2, HIGH);
	bcm2835_gpio_write(TIME_3, HIGH);
	bcm2835_gpio_write(TIME_4, HIGH);
	bcm2835_gpio_write(TIME_5, HIGH);

	printf("P1_ENABLE ");
	bcm2835_gpio_fsel(P1_ENABLE, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(P1_ENABLE, HIGH);
	InterruptDelay(750, true, DataRead);
	bcm2835_gpio_write(P1_ENABLE, LOW);

	printf("P2_ENABLE ");
	bcm2835_gpio_fsel(P2_ENABLE, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(P2_ENABLE, HIGH);
	InterruptDelay(750, true, DataRead);
	bcm2835_gpio_write(P2_ENABLE, LOW);

	printf("P3_ENABLE ");
	bcm2835_gpio_fsel(P3_ENABLE, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(P3_ENABLE, HIGH);
	InterruptDelay(750, true, DataRead);
	bcm2835_gpio_write(P3_ENABLE, LOW);

	bcm2835_gpio_write(TIME_1, LOW);
	bcm2835_gpio_write(TIME_2, LOW);
	bcm2835_gpio_write(TIME_3, LOW);
	bcm2835_gpio_write(TIME_4, LOW);
	bcm2835_gpio_write(TIME_5, LOW);

	printf("- OK\n\n");


	printf("main(): Starting serial port thread...\n");
	//DataRead->StatusByte = 1;
	DataRead.StatusByte = 420;
	pthread_create(&ser, NULL, SerialThread, DataReadPtr);

	printf("main(): SerialThread spawned. Waiting for thread to complete spawning...\n");
	InterruptDelay(5000, true, DataRead);

	printf("main(): Sanity check: Read StatusByte from SerialThread, should be 1337: %d\n", DataReadPtr->StatusByte);

	printf("\amain(): !!! MAKE SURE YOU TEST PLAYER INPUTS BEFORE STARTING GAME !!!\n\nmain(): Good luck - here we go, into the Jeopardy round...\n\n");

	printf("main(): debug: using interval %d as divisor for InterruptDelay(). Change MS_DIVISOR to change sampling rate.\n\n", MS_DIVISOR); 

	//printf("main(): this will probably crash; freeing DataRead\n");
	//free(DataRead);

	printf("main(): DataRead.StatusByte is %d\n", DataRead.StatusByte);

	while(1)
        {
                /* Set these variables to 0 so we start fresh
                   every time the Enabler switch is turned on */
                P1Lockout = 0;
                P2Lockout = 0;
                P3Lockout = 0;

                lockout = 0;//bcm2835_gpio_lev(ENABLER);
                //bcm2835_gpio_write(ENABLER_LED, LOW);

                /* bcm2835 returns 0 when a digital input is triggered,
                   so we only enter the loop when the DI is 0 */
                while(!lockout)
                {
                        value1 = bcm2835_gpio_lev(INPUT1);
                        value2 = bcm2835_gpio_lev(INPUT2);
                        value3 = bcm2835_gpio_lev(INPUT3);
                        lockout = 0;//bcm2835_gpio_lev(ENABLER);

			printf("main(): DataRead.StatusByte is %d\n", DataRead.StatusByte);
			printf("main(): DataReadPtr->StatusByte is %d\n", DataReadPtr->StatusByte);
			InterruptDelay(10, false, DataRead);

			DataRead.StatusByte = DataReadPtr->StatusByte;

			printf("main(): DataRead.StatusByte = DataReadPtr->StatusByte is now %d\n", DataRead.StatusByte);

			//DataRead.StatusByte = DataReadPtr->StatusByte;

                        /* Toggle the Enabler LED on so the operator knows
                           when inputs are being recieved; we can run
                           headless this way */
                        //bcm2835_gpio_write(ENABLER_LED, HIGH);

                        /*if(value1 == 0)
			{
                                if(P1Lockout == 0)
                                {
                                        printf("main(): sending data 1 to StatusByte\n");
					DataReadPtr->StatusByte = 49;
					P1Lockout = GetPlayerRingin(1, P1_LED);

					printf("main(): sending data 4 to StatusByte\n");
					DataReadPtr->StatusByte = 52;
                                }
                                else
                                {
                                        printf("main(): P1 already rung in\n");
                                }
                        }

                        if(value2 == 0)
                        {
                                if(P2Lockout == 0)
                                {
                                        P2Lockout = GetPlayerRingin(2, P2_LED);
                                }
                                else
                                {
                                        printf("main(): P2 already rung in\n");
                                }
                        }

                        if(value3 == 0)
                        {
                                if(P3Lockout == 0)
                                {
                                        P3Lockout = GetPlayerRingin(3, P3_LED);
				}
                                else
                                {
                                        printf("main(): P3 already rung in\n");
                                }
                        }*/
                }
        }

	printf("main(): freeing pointer to SerialThread->DataReadPtr\n");
	free(DataReadPtr);

        bcm2835_close();
        return 0;
}

int TTLOpen()
{
	/* Open the TTL device for r/w access */
	printf("Opening TTL device... not yet implemented\n");

	return 0;
}

int TTLClose()
{
	/* Likewise, close the TTL device when exiting */
	printf("Closing TTL device... not yet implemented\n");

	return 0;
}

int TTLWrite()
{
	/* Write data to the TTL device */

	return 0;
}

int TTLRead()
{
	/* Read data from the TTL device */

	return 0;
}

int GetPlayerRingin(int PlayerInput, RPiGPIOPin playerLED)
{
	//SerData *DataRead = malloc(sizeof(SerData));
	//int EarlyTimerCancel = 0;
	SerData Whatever;

	Whatever.StatusByte = 6969;

	printf("GetPlayerRingin(): StatusByte is %d\n", Whatever.StatusByte);

        /* Return value of 1 is stored in the lockout ints in main()
           so we can lock a player out until the Enabler is turned on
           again */
	printf("GetPlayerRingin(): Player %d rung in\n", PlayerInput);

#ifdef MODEL_AB
	/* Use the old single-LED indicator logic for 26-pin devices. */
	bcm2835_gpio_write(LOCKOUT_ASSERT, HIGH);
	bcm2835_gpio_write(playerLED, HIGH);
	InterruptDelay(5000, false, Whatever);
	bcm2835_gpio_write(LOCKOUT_ASSERT, LOW);
	bcm2835_gpio_write(playerLED, LOW);
#endif
#ifdef MODEL_BPLUS
	/* Use the new multi-LED logic for 40-pin devices. */

	/* Check to see which player rang in and set their countdown
	   enable relay appropriately. */
	switch(PlayerInput)
	{
		case 1:
			bcm2835_gpio_write(P1_ENABLE, HIGH);
			break;
		case 2:
			bcm2835_gpio_write(P2_ENABLE, HIGH);
			break;
		case 3:
			bcm2835_gpio_write(P3_ENABLE, HIGH);
			break;
		default:
			printf("GetPlayerRingin(): WARNING: Tried to set unknown PlayerInput %d HIGH\n", PlayerInput);
			break;
	}

	/* Now turn on the countdown LEDs and start the countdown! */
	bcm2835_gpio_write(TIME_1, HIGH);
	bcm2835_gpio_write(TIME_2, HIGH);
	bcm2835_gpio_write(TIME_3, HIGH);
	bcm2835_gpio_write(TIME_4, HIGH);
	bcm2835_gpio_write(TIME_5, HIGH);	/* O O O O O O O O O */
	InterruptDelay(1000, false, Whatever);

	bcm2835_gpio_write(TIME_5, LOW);	/* - O O O O O O O - */
	InterruptDelay(1000, false, Whatever);

	bcm2835_gpio_write(TIME_4, LOW);	/* - - O O O O O - - */
	InterruptDelay(1000, false, Whatever);

	bcm2835_gpio_write(TIME_3, LOW);	/* - - - O O O - - - */
	InterruptDelay(1000, false, Whatever);

	bcm2835_gpio_write(TIME_2, LOW);	/* - - - - O - - - - */
	InterruptDelay(1000, false, Whatever);

	bcm2835_gpio_write(TIME_1, LOW);	/* - - - - - - - - - */
	InterruptDelay(1000, false, Whatever);

	/* Switch one more time to make sure you disable the player enable */
	switch(PlayerInput)
	{
		case 1:
			bcm2835_gpio_write(P1_ENABLE, LOW);
			break;
		case 2:
			bcm2835_gpio_write(P2_ENABLE, LOW);
			break;
		case 3:
			bcm2835_gpio_write(P3_ENABLE, LOW);
			break;
		default:
			printf("GetPlayerRingin(): WARNING: Tried to set unknown PlayerInput %d LOW\n", PlayerInput);
			break;
	}
#endif

	printf("GetPlayerRingin(): Player %d Time expired!\n", PlayerInput);

	return 0;
}

void ClearCountdownLights(int player)
{

}

void *SerialThread(void *thread)
{
	SerData *statbyte=(SerData *)thread;

	statbyte->StatusByte = 1337;
	int LastStatusByte = 0;

	int countval = 10;

	int fd;
	char buf[255];
	char *bufptr;

	bufptr = buf;

	char DataToSend[2];
	char DataToCompare[2];
	int RetValue;

	struct termios options;

	printf("SerialThread(): Hello from our serial thread!\n");

	printf("SerialThread(): Attempting to open /dev/ttyS0...");
	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	if(fd == -1)
	{
		printf("SerialThread(): failed to open /dev/ttyS0 - error %d %s\n", errno, strerror(errno));
		printf("SerialThread(): thread will now go into infinite loop\n");

		while(1) { }
	}
	else
	{
		printf(" - OK\n");
		fcntl(fd, F_SETFL, 0 | O_NONBLOCK);

		printf("SerialThread(): Setting TTY options\n");
		tcgetattr(fd, &options);

		printf("SerialThread(): Setting 9600BPS... ");
		cfsetispeed(&options, B9600);
		cfsetospeed(&options, B9600);
		printf("- OK\n");

		printf("SerialThread(): Setting 8N1 plus misc TTY options... ");
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;
		options.c_cflag &= ~CRTSCTS;
		options.c_cc[VMIN] = 1;
		options.c_cc[VTIME] = 5;
		options.c_cflag |= CREAD | CLOCAL;
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		printf("- OK\n");

		write(fd, "SReady\r\n", 8);
		memset(buf, 0, sizeof(buf));

		printf("SerialThread(): All serial setup complete, entering data loop\n");
		printf("SerialThread(): TODO!!!!!!!!! Give me a way to exit this loop and kill the thread gracefully\n");

		while(1)
		{
			//printf("SerialThread(): starting if(read)\n");

			read(fd, bufptr, buf + sizeof(buf) - bufptr - 1); //just slam data into the buffer, who cares if the data or device is ready?

			bufptr = buf;

			strcpy(DataToCompare, buf);
			RetValue = strcmp(DataToCompare, buf);

			if(LastStatusByte == statbyte->StatusByte) 
			{
			//	printf("SerialThread(): data error in read()! %d %s\n", errno, strerror(errno));


                                switch(buf[0])
                                {
                                        case 33: // MCP sends pairing request, text value is !
                                                printf("SerialThread(): if LSB = StatByte: received pairing request from MCP, sending ack\n");
                                                write(fd, "@", 1);

		                                memset(buf, 0, sizeof(buf));
                                                break;
                                        case 55: // MCP sends Player 1 Correct/Incorrect Lightbar term request, character 7
                                                printf("SerialThread(): received Player 1 lightbar term request, killing countdown\n");
                                                statbyte->StatusByte = 7;
                                                break;
                                        case 56: // Player 2 correct/incorrect, chr 8
                                                printf("SerialThread(): received Player 2 lightbar term request, killing countdown\n");
                                                //statbyte->StatusByte = 8;
                                                break;
                                        case 57: // Player 3 correct/incorrect, chr 9
                                                printf("SerialThread(): received Player 3 lightbar term request, killing countdown\n");
                                                //statbyte->StatusByte = 9;
                                                break;

					default:
						break;
				}

				LastStatusByte = statbyte->StatusByte;
				memset(buf, 0, sizeof(buf));

				//printf("SerialThread(): LSB: %d SB: %d\n", LastStatusByte,statbyte->StatusByte);

			}
			else
			{
				/* if the data buffer isn't null, start parsing the logic!
				   first we start by proessing outgoing messages... */

				//printf("SerialThread(): got data: %s\n", buf);

                                //printf("SerialThread(): debug: statbyte->StatusByte is %d\n", statbyte->StatusByte);

                                switch(statbyte->StatusByte)
                                {
                                        case 49: // Player 1 ring-in
                                                printf("SerialThread(): recieved byte 1 on StatusByte, sending start countdown to MCP\n");
                                                strcpy(DataToSend, "1");
                                                //write(fd, "A", 1)
                                                break;
                                        case 50: // Player 2 ring-in
                                                printf("SerialThread(): received byte 2 on StatusByte, sending start countdown to MCP\n");
                                                //DataToSend = '2';
                                                break;
                                        case 51: // Player 3 ring-in
                                                printf("SerialThread(): received byte 3 on StatusByte, sending start countdown to MCP\n");
                                                //DataToSend = '3';
                                                break;
                                        case 52: // Player 1 Expired
                                                printf("SerialThread(): recieved byte 4 on StatusByte, sending time expired to MCP\n");
                                                //DataToSend = '4';
						strcpy(DataToSend, "4");
                                                break;
                                        case 53: // Player 2 Expired
                                                printf("SerialThread(): received byte 5 on StatusByte, sending time expired to MCP\n");
                                                //DataToSend = '5';
                                                break;
                                        case 54: // Player 3 Expired
                                                printf("SerialThread(): recieved byte 6 on StatusByte, sending time expired to MCP\n");
                                                //DataToSend = '6';
                                                break;
                                        default:
                                                //printf("SerialThread(): Unknown data in statbyte->StatusByte! %d\n", statbyte->StatusByte);
                                                break;
                                }

				/* and then process incoming messages from MCP 
				switch(buf[0])
				{
					case 33: // MCP sends pairing request, text value is !
						printf("SerialThread(): received pairing request from MCP, sending ack\n");
						write(fd, "@", 1);
						break;
					case 55: // MCP sends Player 1 Correct/Incorrect Lightbar term request, character 7
						printf("SerialThread(): received Player 1 lightbar term request, killing countdown\n");
						statbyte->StatusByte = 7;
						break;
					case 56: // Player 2 correct/incorrect, chr 8
						printf("SerialThread(): received Player 2 lightbar term request, killing countdown\n");
						statbyte->StatusByte = 8;
						break;
					case 57: // Player 3 correct/incorrect, chr 9
						printf("SerialThread(): received Player 3 lightbar term request, killing countdown\n");
						statbyte->StatusByte = 9;
						break;
					default:
						//printf("SerialThread(): got unknown data: %s\n", buf);
						break;
				}*/

				/* and before we send data back to the MCP, check we haven't send the data already lest we flood the MCP! */
				//if(LastStatusByte != statbyte->StatusByte)
				//{
					LastStatusByte = statbyte->StatusByte;

					write(fd, DataToSend, 1);
				//}

				/*if(buf[0] == 102)
				{
					write(fd, "OK\r\n", 4);
				}
				else
				{
					printf("SerialThread(): unknown data: %s\n", buf);
				}*/

				/* and clear the buffer so we don't do logic on old data! */
				memset(buf, 0, sizeof(buf));
			}
		}
	}

	/*while(1)
	{
		//printf("countval: %d | statusbyte: %d\n", countval, statbyte->StatusByte);

		countval++;
		statbyte->StatusByte = countval;

		if(countval > 200000)
		{
			countval = 10;
			statbyte->StatusByte = countval;
		}

		/*switch(countval)
		{
			case 200000:
			{
				countval = 10;
				statbyte->StatusByte = countval;
				break;
			}
			defaut:
			{
				countval++;
				statbyte->StatusByte = countval;
				break;
			}
		}*
	}*/
}

void InterruptDelay(int milliseconds, bool selftest, SerData dataread)
{
        /* This function exists so the operator can cancel a player's
           input without having to wait for the timer to expire. This
           keeps things running fast. */
        uint8_t oi;
        int IDelay;

	//SerData *DataRead2 = malloc(sizeof(SerData)); //create pointer to the serialthread's StatusByte

        for(IDelay = 0; IDelay < (milliseconds / 10); IDelay = IDelay + 1)
        {
                bcm2835_delay(10);
                oi = 1; //bcm2835_gpio_lev(OPERATOR_INTERRUPT);

		//printf("InterruptDelay(): StatusByte is %d\n",dataread.StatusByte);

                if(oi == 0)
                {
                        printf("InterruptDelay(): Ending countdown - Lockout switch was pressed\n");
                        break;
                }
		else
		{
			if(selftest == false)
			{
				switch(dataread.StatusByte)
				{
					case 7:
						break;
					case 8:
						break;
					case 9:
						break;
					default:
						break;
				}

				if(dataread.StatusByte != 0)
				{
					printf("InterruptDelay(): Ending countdown - Recieved data %d on StatusByte\n", dataread.StatusByte);
					break;
				}
			}
		}
        }

	return 0;
}

void CheckIfRoot()
{
	/* bcm2835 now supports root-less execution.
	   We still will prefer running as root, however.
	   Inform the operator that we are not running as root,
	   or that there's a root UID mismatch. */
	printf("CheckIfRoot(): Checking if root/sudo... ");

	printf("UID %i ", getuid());

	if(getuid() != ROOT_UID)
	{
		printf("- FAIL\n\n");

		printf("You do not appear to be running as root. Program will still continue, but you may experience abnormal program behaviour.\nIf you are runing as root and are still seeing this message, change ROOT_UID to the listed UID above and recompile.\n\n");
		//printf("You do not appear to be running as root.\nPlease launch as root and try again.\nIf you are running as root but have a different UID, change ROOT_UID to your UID, recompile, and try again.\n\n");

		//exit(0);
	}
	else
	{
		printf("- OK\n");
	}
}

void CleanupAndClose()
{
	/* Catch ^C and make sure all LEDs are turned off before
	   exiting the program. */
	signal(SIGINT, CleanupAndClose);
	printf("\n\nCleanupAndClose(): Terminating... \n");

/*	printf("ENABLER_LED ");
	bcm2835_gpio_write(ENABLER_LED, LOW);
	printf("OFF ");*/

	printf("P1_LED ");
	bcm2835_gpio_write(P1_LED, LOW);
	printf("- OFF\n");

	printf("P2_LED ");
	bcm2835_gpio_write(P2_LED, LOW);
	printf("- OFF\n");

	printf("P3_LED ");
	bcm2835_gpio_write(P3_LED, LOW);
	printf("- OFF\n");

	TTLClose();

	printf("CleanupAndClose(): All systems terminated OK\n\n");

	printf("CleanupAndClose(): Thanks for playing Jeopardy!\n\n");

	exit(0);
}
