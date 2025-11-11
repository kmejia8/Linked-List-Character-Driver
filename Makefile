# Karla Mejia
# ECE 446 HW 5
# Device Driver Development
# Makefile for HW 5

obj-m += runner_mod.o

USER_PROG = runner_space
CC = gcc

# building kernel module and user program
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) $(USER_PROG).c -o $(USER_PROG)

# cleans up files
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(USER_PROG)
