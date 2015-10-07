CC = gcc
CFLAGS = -O2 -g -Wall -Wextra -Wformat=2 `pkg-config --cflags glib-2.0`
LDLIBS = `pkg-config --libs glib-2.0`
PORT =  /labs/tsam15/my_port

all: httpd

clean:
	rm -f *.o *~

distclean: clean
	rm -f httpd

run: distclean ; make
	./httpd 4074
	
