// sudo mknod -m 0666 /dev/ruslandev c 244 0

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

// File system
#include <linux/fs.h>
// put_user
#include <asm/uaccess.h>

#include <linux/kernel.h>

// Debugfs
#include <linux/debugfs.h>

// Kobject
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>

static struct dentry * dir = 0;
static u32 hello = 0;
static u32 sum = 0;

static int add_write_op(void * data, u64 value){
	sum += value;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(add_fops, NULL, add_write_op, "%llu\n");
// Debugfs ends



MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Ruslan Kuzmin");

#define IOCTL_SET_MSG 1

#define IOCTL_GET_MSG 2

static int iparam = 0;
module_param( iparam, int,0);

static int k = 0;
module_param_named( nparam , k , int,0);

static char * sparam;
module_param(sparam, charp , 0);

static int aparam[] = { 0 , 0 , 0 , 0 , 0 };
static int arnum = sizeof(aparam) / sizeof( aparam[0] );

module_param_array( aparam , int , &arnum ,S_IRUGO | S_IWUSR);

#define FIXLEN 5

static char s[ FIXLEN ] = "";
module_param_string( cparam , s , sizeof(s) , 0);

// Char dev begin

#define SUCCESS 0
#define DEVICE_NAME "ruslandev"
#define BUF_LEN 80

static int Major;
static int Device_Open = 0;

static char msg[BUF_LEN];
static char * msg_Ptr;

static int device_open(struct inode * inode,struct file * file){
	static int counter = 0;
	if(Device_Open)
		return -EBUSY;
	Device_Open++;
	sprintf(msg,"I already told you %d times Hello world!\n",counter++);
	msg_Ptr=msg;
	try_module_get(THIS_MODULE);
	return SUCCESS;
}

static int device_release(struct inode * inode,struct file * file){
	Device_Open--;
	module_put(THIS_MODULE);
	return SUCCESS;
}


static ssize_t device_read(struct file * filp,char * buffer,size_t length, loff_t * offset){
	int bytes_read=0;
	if(*msg_Ptr == 0)
		return 0;

	while(length && *msg_Ptr){
		put_user(*(msg_Ptr)++,buffer++);
		length--;
		bytes_read++;
	}
	return bytes_read;
}

static long device_ioctl(struct file * file,unsigned int ioctl_num,unsigned long ioctl_param) {
	switch(ioctl_num) {
		case IOCTL_SET_MSG:
			printk(KERN_INFO "IOCTL_SET_MSG passed");
		break;
		case IOCTL_GET_MSG:
			printk(KERN_INFO "IOCTL_GET_MSG passed");
		break;
		default:
			printk(KERN_INFO "DEFAULT_SET_MSG passed");
		break;		
	}
	return SUCCESS;
}


static ssize_t device_write(struct file * filp,const char * buff,size_t len, loff_t * off){
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
	return -EINVAL;
}


static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
	.unlocked_ioctl = device_ioctl
};



//--------------------------------------------------------- Kobject and sysfs -----------------------------------------------------------------
static struct kobject * example_kobject;

static int kobject_example(void){

	example_kobject = kobject_create_and_add("kobject_example", kernel_kobj);
}
//--------------------------------------------------------- Kobject and sysfs end -------------------------------------------------------------


irqreturn_t interrupt_handler(int interrpupt, void * notused ){
	return IRQ_HANDLED;
}

static unsigned int interrupt_handler_result;

static int hello_init(void){
	printk(KERN_ALERT "Hello,world!\n");

	
	int j;
	printk( "iparam = %d\n" , iparam);
	printk( "nparam = %d\n", k);
	printk( "sparam = %s\n", sparam);
	for( j = 0 ; j < arnum ; ++j ){
		printk("aparam %d = %d \n",j,aparam[j]);
	}


	Major = register_chrdev(0,DEVICE_NAME,&fops);
	if( Major < 0 ){
		printk( KERN_ALERT "Registering char device failed with %d\n", Major);
		return -EINVAL;
	}
	printk( KERN_ALERT "Device created =  %d\n", Major);


	// Debugfs begin
	struct dentry * junk;
	dir = debugfs_create_dir("example1",0);
	if(!dir) {
		printk(KERN_ALERT "debugfs_example1: failed to create /sys/kernel/debug/example1\n");
		return -1;
	}

	junk = debugfs_create_u32("hello", 0666, dir , &hello);
	if(!junk) {
		printk(KERN_ALERT "debugfs_example1: failed to create /sys/kernel/debug/example1/hello\n");
		return -1;
	}


	junk = debugfs_create_file("add",0222,dir,NULL,&add_fops);
	if(!junk) {
		printk(KERN_ALERT "debugfs_example1: failed to create /sys/kernel/debug/example1/add\n");
		return -1;
	}

	junk = debugfs_create_u32("sum",0444,dir,&sum);
	if(!junk) {
		printk(KERN_ALERT "debugfs_example1: failed to create /sys/kernel/debug/example1/sum\n");
		return -1;
	}

	kobject_example();

	interrupt_handler_result = request_irq(2,interrupt_handler,0,"MY INTERRUPT",NULL);
	

	if(interrupt_handler_result == 0){
		printk(KERN_ALERT "have interrupt");
		
	} else {
		printk(KERN_ALERT "failed get interrupt");
	}

	irq_set_irq_type( interrupt_handler_result , IRQ_TYPE_EDGE_RISING );

	// Debugfs end
	return 0;
}


static void hello_exit(void){
	printk(KERN_ALERT "Goodbuy ,cruel world!\n");

	free_irq(interrupt_handler_result,NULL);

	unregister_chrdev(Major , DEVICE_NAME);

	debugfs_remove_recursive(dir);

	kobject_put(example_kobject);

	return;
}



module_init(hello_init);
module_exit(hello_exit);
