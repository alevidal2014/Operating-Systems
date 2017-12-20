/*---Start of mycall.c---*/
#include <linux/linkage.h>
#include <linux/time.h>
#include <linux/sched.h>

asmlinkage long sys_alejandro_vidal(int pantherid)
{

struct timeval now;	//Time in seconds since 1/1/1970 00:00:00
struct tm broken;	//Time broken in human formatt
long year;		//Final year result
int month;		//Final Month result

printk("sys_alejandro_vidal called from process %i with panther id %d\n", current->pid, pantherid);

do_gettimeofday(&now);
time_to_tm(now.tv_sec, 0, &broken);
year = broken.tm_year + 1900;
month = broken.tm_mon +1;

printk("The Current Time is: %dh  %dm  %ds  %d/%d/%ld\n",broken.tm_hour, broken.tm_min, broken.tm_sec, month, broken.tm_mday, year );
return 0;
}
/*---End of mycall.c---*/
