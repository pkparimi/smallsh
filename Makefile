main:
	gcc -std=gnu99 -Wall -g -o smallsh smallsh.c

clean:
	rm -f smallsh

debug:
	gcc -std=gnu99 -Wall -g -o smallsh smallsh.c
	valgrind -s --leak-check=yes --track-origins=yes --show-reachable=yes ./smallsh

test:
	gcc -std=gnu99 -Wall -g -o smallsh smallsh.c
	chmod +x ./p3testscript
	$ ./p3testscript > mytestresults 2>&1 

testclean:
	rm -f smallsh
	rm -f junk
	rm -f junk2
	rm -f mytestresults