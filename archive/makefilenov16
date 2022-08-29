# Filename: gpio.c
# Author: Nathan Phillip Brink (binki@ohnopub)
# Website: http://www.beige-box.net/
# Description: Simple ugly Makefile
#
# The following source code is (C) 2014 The Little Beige Box
# and is released as open-source software under the terms
# contained in LICENSE.txt. In the event you did not
# recieve a copy of the license, please contact the email
# address below for a free digital copy.
#
# support@beige-box.com

.PHONY: all clean

LIBS = -lbcm2835
OBJ = gpio.o

all: jeopardy-ringin
jeopardy-ringin: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o "$(@)" $(OBJ) $(LIBS)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o "$(@)" "$(<)"

clean:
	rm -f $(OBJ) jeopardy-ringin
