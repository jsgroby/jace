
debug:
	gcc *.c -g -Wall -Werror -o jace 

profile-guided:
	gcc *.c -o jace -O3 -Wall -fprofile-use 

release:
	gcc *.c -o jace 

release-opt02:
	gcc *.c -o jace -O2

release-opt03-preferred:
	gcc *.c -o jace -O3 

release-opt03-fastmath:
	gcc *.c -o jace -O3 -ffast-math

