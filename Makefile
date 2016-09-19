all: 320sh

320sh: 320sh.c
	gcc -Wall -Werror -g -o 320sh 320sh.c konrad.c charles.c

clean:
	rm -f *~ *.o 320sh
