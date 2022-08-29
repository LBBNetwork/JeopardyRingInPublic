/* Code for Jeopardy Ring-in Device, Mk.III

   Hardware configuration:
   * Raspberry Pi (Please let me know if the GPIO pins
     differ from each revision - I checked with bcm2835.h
     and tried as best I could but I only have a Rev2
     Model B)
   * Three momentary contact switches on pins 4, 17, 21/27
      amd 22
   * Three LEDs (plus appropriate resistors) on pins 18,
      23, 24, and 25

   Software configuration:
   * Requires the bcm2835 library for GPIO. Get it from:
     http://www.airspayce.com/mikem/bcm2835/

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

   The following source code is (c) 2014-2020 The Little Beige Box
   and is released as open-source software under the terms
   contained in LICENSE.txt. In the event you did not
   recieve a copy of the license, please contact the email
   address below for a free digital copy.

   support@beige-box.com
*/

#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define ROOT_UID 0

#define INPUT1 RPI_GPIO_P1_07			//Pin 4
#define INPUT2 RPI_GPIO_P1_11			//Pin 17
#define INPUT3 RPI_V2_GPIO_P1_13		//Pin 27 (Rev2)
//#define INPUT3 RPI_GPIO_P1_13			//Pin 21 (Rev1)
#define INPUT4 RPI_GPIO_P1_15			//Pin 22
#define P1_LED RPI_GPIO_P1_12			//Pin 18
#define P2_LED RPI_GPIO_P1_16			//Pin 23
#define P3_LED RPI_GPIO_P1_18			//Pin 24
#define P4_LED RPI_GPIO_P1_22			//Pin 25


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

int GetPlayerRingin(int PlayerInput, RPiGPIOPin playerLED);
void InterruptDelay(int milliseconds);
void CheckIfSamba();
void CheckIfRoot();
void CleanupAndClose();

int main()
{
	/* Hook ^C */
	signal(SIGINT, CleanupAndClose);

	/* Clear screen and display startup text */
	printf("\033[H\033[J");
	printf("Jeopardy Ring-In Device Mk. III Prototype\nCopyright (c) 2014-2019 The Little Beige Box\nwww.beige-box.com\n\nSELF TEST START\n\n");

        uint8_t lockout, value1, value2, value3;
        int P1Lockout, P2Lockout, P3Lockout;

	/* Check if samba server is running */
	CheckIfSamba();

	/* Check if current UID is the ROOT_UID */
	CheckIfRoot();

        if(!bcm2835_init())
                return 1;

	InterruptDelay(750);

        /* Set up the GPIO pins for input */
	printf("Setting up GPIO input... ");

	bcm2835_gpio_fsel(INPUT1, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(INPUT1, BCM2835_GPIO_PUD_UP);
	printf("INPUT1 ");

        bcm2835_gpio_fsel(INPUT2, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(INPUT2, BCM2835_GPIO_PUD_UP);
	printf("INPUT2 ");

        bcm2835_gpio_fsel(INPUT3, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(INPUT3, BCM2835_GPIO_PUD_UP);
	printf("INPUT3 ");

/*        bcm2835_gpio_fsel(ENABLER, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(ENABLER, BCM2835_GPIO_PUD_UP);
	printf("ENABLER ");

        bcm2835_gpio_fsel(OPERATOR_INTERRUPT, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(OPERATOR_INTERRUPT, BCM2835_GPIO_PUD_UP);
	printf("OPERATOR_INTERRUPT ");*/

	printf("- OK\n");

        /* Set up the GPIO pins for LED output & 
           perform a self-test of all LEDs */
	printf("Setting up GPIO LEDs... ");

/*	bcm2835_gpio_fsel(ENABLER_LED, BCM2835_GPIO_FSEL_OUTP);
	printf("ENABLER_LED ");
	bcm2835_gpio_write(ENABLER_LED, HIGH);
	InterruptDelay(750);
	bcm2835_gpio_write(ENABLER_LED, LOW);*/


        bcm2835_gpio_fsel(P1_LED, BCM2835_GPIO_FSEL_OUTP);
        printf("P1_LED ");
	bcm2835_gpio_write(P1_LED, HIGH);
	InterruptDelay(750);
	bcm2835_gpio_write(P1_LED, LOW);


	bcm2835_gpio_fsel(P2_LED, BCM2835_GPIO_FSEL_OUTP);
	printf("P2_LED ");
	bcm2835_gpio_write(P2_LED, HIGH);
	InterruptDelay(750);
	bcm2835_gpio_write(P2_LED, LOW);


        bcm2835_gpio_fsel(P3_LED, BCM2835_GPIO_FSEL_OUTP);
	printf("P3_LED ");
	bcm2835_gpio_write(P3_LED, HIGH);
	InterruptDelay(750);
	bcm2835_gpio_write(P3_LED, LOW);

	printf("- OK\n\n");

	printf("\a!!! MAKE SURE YOU TEST PLAYER INPUTS BEFORE STARTING GAME !!!\n\nGood luck - here we go, into the Jeopardy round...\n\n");

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

                        /* Toggle the Enabler LED on so the operator knows
                           when inputs are being recieved; we can run
                           headless this way */
                        //bcm2835_gpio_write(ENABLER_LED, HIGH);

                        if(value1 == 0)
                        {
                                if(P1Lockout == 0)
                                {
                                        P1Lockout = GetPlayerRingin(1, P1_LED);
                                }
                                else
                                {
                                        printf("P1 already rung in\n");
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
                                        printf("P2 already rung in\n");
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
                                        printf("P3 already rung in\n");
                                }
                        }
                }
        }

        bcm2835_close();
        return 0;
}

int GetPlayerRingin(int PlayerInput, RPiGPIOPin playerLED)
{
        /* Return value of 1 is stored in the lockout ints in main()
           so we can lock a player out until the Enabler is turned on
           again */
	printf("Player %d rung in\n", PlayerInput);
	bcm2835_gpio_write(playerLED, HIGH);
	InterruptDelay(5000);
	bcm2835_gpio_write(playerLED, LOW);
	return 0;
}

void InterruptDelay(int milliseconds)
{
        /* This function exists so the operator can cancel a player's
           input without having to wait for the timer to expire. This
           keeps things running fast. */
        uint8_t oi;
        int IDelay;

        for(IDelay = 0; IDelay < milliseconds; IDelay = IDelay + 1)
        {
                bcm2835_delay(1);
                oi = 1; //bcm2835_gpio_lev(OPERATOR_INTERRUPT);

                if(oi == 0)
                {
                        printf("Ending countdown - OI switch was pressed\n");
                        break;
                }
        }
}

void CheckIfSamba()
{
	/* Checks if Samba server is running. Req'd for Jeopardy 2.0
	   to know who rung in. Currently a stub function but will
	    do something later in development */

	printf("Checking if Samba server is running...  Not yet implemented\n");
}

void CheckIfRoot()
{
	/* root is required as this program requires GPIO access.
	   This function checks if the current user is root and
	   aborts if false. */
	printf("Checking if root/sudo... ");

	printf("UID %i ", getuid());

	if(getuid() != ROOT_UID)
	{
		printf("- FAIL\n\n");

		printf("You do not appear to be running as root.\nPlease launch as root and try again.\nIf you are running as root but have a different UID, change ROOT_UID to your UID, recompile, and try again.\n\n");

		exit(0);
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
	printf("\n\nTerminating... ");

/*	printf("ENABLER_LED ");
	bcm2835_gpio_write(ENABLER_LED, LOW);
	printf("OFF ");*/

	printf("P1_LED ");
	bcm2835_gpio_write(P1_LED, LOW);
	printf("OFF ");

	printf("P2_LED ");
	bcm2835_gpio_write(P2_LED, LOW);
	printf("OFF ");

	printf("P3_LED ");
	bcm2835_gpio_write(P3_LED, LOW);
	printf("OFF ");

	printf("- OK\n\n");

	printf("Thanks for playing Jeopardy!\n\n");

	exit(0);
}
