8/29/22: Oh boy I finally remembered my github login. Gonna push the Mk. V source soon.

Oops I accidentially pushed PII to the other repo, well, time to delete

Code for Jeopardy Ring-in Device Mk. IV

Useful code if you have a Raspberry Pi and would like to use it as a ring-in device for game shows,
or use it as a starting point to perhaps figure out how GPIO works on the RPi.

Hardware configuration:

* to be revised
* TTL pins enabled at 9600 8-N-1

Software configuration:

* Requires the bcm2835 library for GPIO. Get it from:
  http://www.airspayce.com/mikem/bcm2835/
* libpthread
* libncurses (soon...)
* other standard POSIX libraries, read the source code

Compiling:

* Ensure you have gcc and the libraries installed.
  To compile, simply issue this command:

  make

Running:

* We recommend you run the program as root, but it should still run as a normal user.

Have fun!


(thanks binki for the makefile)
