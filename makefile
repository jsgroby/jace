SRCS=$(wildcard *.c)
NAME=jace


all:
	gcc $(SRCS) -o $(NAME) -O2
