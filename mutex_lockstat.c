#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uio.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/time.h>
#include<linux/list.h>
#include<linux/slab.h>
#include<linux/fs.h>
#include <asm/uaccess.h>   
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/sched.h>
#include "lockstat.h"

struct timespec duration;
static int i;
struct lock *head,*lock_list;
void copy_to_file(struct file *f,mm_segment_t fs,lock_node data)
{
		fs = get_fs();
        	set_fs(get_ds());
        	f->f_op->write(f,(char*)&data,sizeof(struct lock_summary), &f->f_pos);
		set_fs(fs);
}
lock_node copy_to_node(struct lock *temp)
{
		lock_node data;
		data.i=temp->i;
		data.pid=temp->pid;  
		strcpy(data.comm,temp->comm);
		data.tm=temp->tm;
		strcpy(data.type,temp->type);
		data.start_sec=temp->start_sec;
		data.start_nsec=temp->start_nsec;
		data.wait_time=temp->wait_time;
		data.res_addr=temp->res_addr;
		return (data);
}
struct lock* allocate_node(int i,struct timespec curr_tm,struct mutex *resource)
{
	struct lock* temp_lock_node;
	temp_lock_node = kmalloc(sizeof(*temp_lock_node), GFP_KERNEL);
	temp_lock_node->i=++i;
	temp_lock_node->tm=curr_tm;
	temp_lock_node->res_addr=resource;
	strcpy(temp_lock_node->comm,current->comm);
	temp_lock_node->pid=current->pid;
	strcpy(temp_lock_node->type,"mutex");
	temp_lock_node->next=NULL;
	return (temp_lock_node);
}
void allocate_head(int i,struct timespec curr_tm,struct mutex *resource)
{
	printk("chek:: %s\n",current->comm);
	head=kmalloc(sizeof(*head),GFP_KERNEL);
	head->i=++i;
	head->tm=curr_tm;
	head->pid=current->pid;
	head->res_addr=resource;
	strcpy(head->type,"mutex");
	strcpy(head->comm,current->comm);
	head->next=NULL;
	lock_list=head;
}

void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long
flags)
{
	struct timespec now;
	getnstimeofday(&now);
	duration.tv_sec=now.tv_sec-duration.tv_sec;
	duration.tv_nsec = now.tv_nsec - duration.tv_nsec;
	lock_list->wait_time=duration.tv_nsec;
}

void handler_pre(struct mutex *resource)
{
	struct lock *temp_lock_node;
	struct timespec curr_tm;
	getnstimeofday(&curr_tm);
	curr_tm.tv_sec=curr_tm.tv_sec+3600*5+30*60;
	if(i<50000)
	{
		if(i==0)
			allocate_head(i,curr_tm,resource);
		else
		{
			temp_lock_node = kmalloc(sizeof(*temp_lock_node), GFP_KERNEL);
			temp_lock_node=allocate_node(i,curr_tm,resource);
			lock_list->next=temp_lock_node;
			lock_list = temp_lock_node;
		}
		i++;	
	}
	getnstimeofday(&duration);
	duration.tv_sec=duration.tv_sec+3600*5+30*60;
	lock_list->start_sec=duration.tv_sec;
	lock_list->start_nsec=duration.tv_nsec;
	jprobe_return();
}

static struct jprobe my_jprobe = {
	.entry = (kprobe_opcode_t *) handler_pre
};

int begin_module(void)
{
	int ret;
	my_jprobe.kp.post_handler = handler_post;
	my_jprobe.kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name("mutex_lock");
	if (!my_jprobe.kp.addr) {
		printk("Couldn't find %s to plant jprobe\n", "mutex_lock");
		return -1;
	}

	if ((ret = register_jprobe(&my_jprobe)) <0) {
		printk("register_jprobe failed, returned %d\n", ret);
		return -1;
	}
	printk("Planted jprobe at %p, handler addr %p\n",
	       my_jprobe.kp.addr, my_jprobe.entry);
	return 0;
}

void end_module(void)
{
	struct file *f;

	lock_node data;
	mm_segment_t fs;  

	unregister_jprobe(&my_jprobe);
	printk("jprobe unregistered\n");
	
	f = filp_open("ab.txt", O_WRONLY|O_CREAT, 0);
	if(f == NULL)
        	printk(KERN_ALERT "filp_open error!!.\n");
    	else
	{
		while(head!=NULL)
		{
		data=copy_to_node(head);
		printk("cmmd:: %s",data.comm);
		copy_to_file(f,fs,data);
		head = head->next;
		}
	}
	filp_close(f,NULL);
}
MODULE_LICENSE("GPL");
module_init(begin_module);
module_exit(end_module);

