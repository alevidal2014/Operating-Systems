/*---Start of mycall.c---*/
#include <linux/linkage.h>
#include <linux/sched.h>

asmlinkage long sys_print_tasks_alejandro_vidal()
{
printk("sys_print_tasks_alejandro_vidal called from process %i\n", current->pid);
printk("Process Information:\n");

struct task_struct *task;	//Process struct

for_each_process(task) //Looping through all the process
{
	struct task_struct *t = task;	//Thread struct
	do{
		printk("Name: %s, Pid: [%d], State: %li, Parent Pid: [%d]\n", t->comm, t->pid, t->state, t->parent->pid);		
	
	}while_each_thread(task, t); //Looping through all the threads
}

return 0;
}
/*---End of mycall.c---*/
