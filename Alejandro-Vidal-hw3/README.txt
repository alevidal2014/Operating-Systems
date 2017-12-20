
/********************************************************
##########################################################
## COP4610 – Principles of Operating Systems – Fall 2017
## 
## Student: Alejandro Vidal – 5913959
## Project: Multithreaded Programming
##
## I Certify that this program code has been written by me
## and no part of it has been taken from any sources.
##########################################################
*********************************************************/


Instructions

Part 1:

In folder 1.1 & 1.2:
Type make async to compile the program without synchronization. Executable name: async
Type make sync to compile the program with synchronization. Executable name: sync

Part 2:

In folder 2
We can find the source ode of our system call and the source code of the program used to test it. Also the patch with the difference between the original kernel and the modified one. 

In order to run the test program we must add a new system call to our kernel named(print_tasks_alejandro_vidal) and compile the kernel. 
Then we can compile our test program using:
	gcc -pthread -D PTHREAD_SYNC -o sync simplethread.c

this will create a sync object which we can used to test our system call. 
	./ sync 3

we can see the threads information using 
	dmesg
