/*Alejandro Vidal PID 5913959*/
/*COP 4610, Fall 2017 */

#include <linux/module.h>

static int hello_init(void)
{
printk("Hello Class, this is Alejandro Vidal\n");
return 0;
}

static void hello_exit(void)
{
printk("Goodbye Class, AV\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Alejandro Vidal");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Greeting Module");