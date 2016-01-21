#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include<stdlib.h>
#include<string.h>
#include "lockstat.h"
#define FILE_mutex "ab.txt"

static int i;
int sleep_time;
typedef struct locks
{
	int count;
	char pname[50];
	char type[10];
	long int start_sec;
	long int start_nsec;
	long int wait_time;
	float cpu_util;
	unsigned int res_addr;
	struct locks *next;
}lock_data;
lock_data *head,*q;

void insert(lock_node read)
{



	lock_data *p;
	int k = 0;

	i++;

	if(head==NULL)
	{
		head=(lock_data*)malloc(sizeof(lock_data));
		head->count=1;
		head->wait_time=0;
		strcpy(head->pname,read.comm);
		strcpy(head->type,read.type);
		head->res_addr=read.res_addr;
		head->start_sec=read.start_sec;
		head->start_nsec=read.start_nsec;
		head->wait_time=read.wait_time;
		head->cpu_util=(head->wait_time*head->count)/sleep_time;
		head->next=NULL;
		q=head;
	}
	else
	{
		
		p=(lock_data*)malloc(sizeof(lock_data));		
		strcpy(p->pname,read.comm);
		strcpy(p->type,read.type);
		p->wait_time=0;
		p->res_addr=read.res_addr;
		p->start_sec=read.start_sec;
		p->start_nsec=read.start_nsec;
		p->wait_time=read.wait_time;
		p->count=1;
		p->cpu_util=(p->wait_time*p->count)/sleep_time;
		p->next=NULL;
		q->next=p;
		q=p;
	}

}

void check(lock_node read)
{
	int flag=0;
	if(head==NULL)
		insert(read);
	else
	{
		lock_data *p;
		p=head;
		while(p!=NULL)
		{
			if(!strcmp(p->pname,read.comm)&&p->res_addr==read.res_addr)
			{
				p->count=p->count+1;
				p->wait_time=((p->wait_time*(p->count-1))+read.wait_time)/p->count;
				p->cpu_util=(p->wait_time*p->count)/sleep_time;
				break;			
			}
		p=p->next;
		}
		if(p==NULL)
			insert(read);
	}
}
void print()
{
	printf("\nCOUNT\t\t\tPROC\t\tWAIT_TIME\t\tSTART_TIME\t\t\tresource\t\tTYPE\n");
	printf("\n--------------------------------------------------------------------------------------------------------------------------------------------\n");
	lock_data *temp;
	temp = head;
	while(temp!=NULL)
	{
		printf("  %d\t %20s\t%15ld\t\t%3.2ld:%3.2ld:%3.2ld:%5.4ld \t\t%15u \t\t %s\n",temp->count,temp->pname,temp->wait_time,((temp->start_sec / 3600)% (24)),
                ((temp->start_sec / 60) % (60)),
                temp->start_sec%60,
                temp->start_nsec / 1000,temp->res_addr,temp->type);
		temp=temp->next;
	}
}

void arg_m(FILE *fp,lock_node read)
{  

	int i=0;
   if(fp==NULL)
		printf("\n Cannot open the MUTEX source file\n");
	while(1)
	{
		if(feof(fp))
			break;
		
		fread(&read,sizeof(struct lock_summary),1,fp);
		check(read);
	}
	
}
void arg_s(FILE *fp,lock_node read)
{
	if(fp==NULL)
		printf("\n Cannot open the SEMAPHORE source file\n");
	while(1)
	{
		if(feof(fp))
			break;
		
		fread(&read,sizeof(struct lock_summary),1,fp);
		check(read);
	}
	
}
void arg_sp(FILE *fp,lock_node read)
{
	if(fp==NULL)
		printf("\n Cannot open the SEMAPHORE source file\n");
	while(1)
	{
		if(feof(fp))
			break;
		
		fread(&read,sizeof(struct lock_summary),1,fp);
		check(read);
	}
	
}

void arg_d(FILE *fp,FILE *fp1,lock_node read,lock_node read1)
{
	int flag;
	flag=-1;
	if(fp==NULL)
		printf("\n Cannot open the MUTEX source file\n");
	if(fp1==NULL)
		printf("\n Cannot open the SEMAPHORE source file\n");
	if(!feof(fp))
	{	
		fread(&read,sizeof(struct lock_summary),1,fp);
	}
	if(!feof(fp1))
		fread(&read1,sizeof(struct lock_summary),1,fp1);
	do
	{
		if(!feof(fp) && !feof(fp1))
		{		
			if(read.start_nsec>read1.start_nsec)
			{
			check(read1);
			if(!feof(fp1))
				fread(&read1,sizeof(struct lock_summary),1,fp1);	
		}
		else
		{
			check(read);
			if(!feof(fp))
			{	
				fread(&read,sizeof(struct lock_summary),1,fp);
			}
		}
		}
	}while(!feof(fp) && !feof(fp1));
	if(read.start_nsec>read1.start_nsec)
	{
		check(read1);
		check(read);
	}
	else
	{
		check(read);
		check(read);
	}
	while(!feof(fp))
	{
		check(read);
		fread(&read,sizeof(struct lock_summary),1,fp);
	}
	while(!feof(fp1))
	{
		check(read1);
		fread(&read1,sizeof(struct lock_summary),1,fp1);
	}

}

int main(int argc, char *argv[])
{
        
	
	FILE *fp,*fp1;
	lock_node read,read1;
	head=NULL;
    enum
    {
        e_get,
        e_clr,
	deaf,
	slp
	
    } option;

    if (argc == 1)
    {
        fprintf(stderr, "Usage: %s [-mutex | -sem  ]\n", argv[0]);
    }
    else if (argc == 2)
    {
	sleep_time=5;
        if (strcmp(argv[1], "-mutex") == 0)
        {
            option = e_get;
        }
        else if (strcmp(argv[1], "-sem") == 0)
        {
            option = e_clr;
        }
	else if (strcmp(argv[1], "-all") == 0)
        {
            option = deaf;
        }
	else
        {
            fprintf(stderr, "Usage: %s [-mutex | -sem  ]\n", argv[0]);
            return 1;
        }
    }
    else if (argc == 3)
    {
	sleep_time=atoi(argv[1]);
        if (strcmp(argv[2], "-mutex") == 0)
        {
            option = e_get;
        }
        else if (strcmp(argv[2], "-sem") == 0)
        {
            option = e_clr;
        }
	else if (strcmp(argv[2], "-all") == 0)
        {
            option = deaf;
        }
	else
        {
            fprintf(stderr, "Usage: %s [-mutex | -sem  ]\n", argv[0]);
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s [-mutex | -sem  ]\n", argv[0]);
        return 1;
    }
    
    switch (option)
    {
        case e_get:
		system("insmod mutex_lockstat.ko");
		sleep(sleep_time);
		system("rmmod mutex_lockstat.ko");
		fp=fopen("ab.txt","r");
		arg_m(fp,read);
		remove("ab.txt");
            break;
        case e_clr:
		system("insmod sem_lockstat.ko");
		sleep(sleep_time);
		system("rmmod sem_lockstat.ko");
		fp=fopen("ab2.txt","r");
		arg_s(fp,read);
		remove("ab2.txt");
            break;
	case deaf:
		system("insmod sem_lockstat.ko");		
		system("insmod mutex_lockstat.ko");
		sleep(sleep_time);
		system("rmmod mutex_lockstat.ko");
		system("rmmod sem_lockstat.ko");
		
		fp=fopen("ab.txt","r");
		fp1=fopen("ab2.txt","r");
		arg_d(fp,fp1,read,read1);
		remove("ab2.txt");
		remove("ab.txt");
		break;
        default:
            break;
    }
	fclose(fp);
	print();
 

    return 0;
}
