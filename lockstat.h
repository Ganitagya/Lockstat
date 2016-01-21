#ifndef DISPLAY_1_H
#define DISPLAY_1_H
#include <linux/ioctl.h>
typedef struct lock
{
	int i;
	unsigned int pid;
	long int wait_time;
	char type[10];
	long int start_sec;
	long int start_nsec;	
	unsigned int res_addr;	
	char comm[30];
	struct timespec tm;
	struct lock *next; 
}lock1;
typedef struct lock_summary
{
	int i;
	unsigned int pid;
	long int wait_time;	
	long int start_sec;
	char type[10];
	long int start_nsec;
	unsigned int res_addr;	
	char comm[30];
	struct timespec tm;
}lock_node;

typedef struct
{
    unsigned long int hr;
    unsigned long int min;
    unsigned long int sec;
    unsigned long int nsec;
    char proc[50];
    lock1 addr;
} query_arg_t;
 
#define DISPLAY_G _IOR('q', 1, query_arg_t *)
#define DISPLAY_C _IOR('q', 2, query_arg_t *)
 
#endif
