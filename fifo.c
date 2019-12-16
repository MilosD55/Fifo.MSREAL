
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#define BUFF_SIZE 20
MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

DECLARE_WAIT_QUEUE_HEAD(readQ);
DECLARE_WAIT_QUEUE_HEAD(writeQ);
struct semaphore sem;


int fifo[16];
int pos_upis = 0;
int pos_citanja=0;
int br_clanova=0;
int endRead = 0;
int i = 0;
char hord[4];
int h=1;
char hex[]="hex";
char dec[]="dec";
int n=1;
char num[]="num=";

int fifo_open(struct inode *pinode, struct file *pfile);
int fifo_close(struct inode *pinode, struct file *pfile);
ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t fifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = fifo_open,
	.read = fifo_read,
	.write = fifo_write,
	.release = fifo_close,
};


int fifo_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened fifo\n");
		return 0;
}

int fifo_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed fifo\n");
		return 0;
}

ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	long int len=0;

	if (endRead==n){
		endRead = 0;
		return 0;
	}

	/*if(down_interruptible(&sem))
		return -ERESTARTSYS;
	while(pos_upis == 0)
	{
		up(&sem);
		if(wait_event_interruptible(readQ,(pos_upis>0)))
			return -ERESTARTSYS;
		if(down_interruptible(&sem))
			return -ERESTARTSYS;
	}*/


		if(br_clanova > 0){

			if( h == 1) {
				len = scnprintf(buff,BUFF_SIZE, "0x""%x\n", fifo[pos_citanja]);
				ret = copy_to_user(buffer, buff, len);
				printk("Succesfully read: %s\n",buff);

			}
			else if(h == 0){
				len = scnprintf(buff,BUFF_SIZE," %d \n",fifo[pos_citanja]);
				ret = copy_to_user(buffer, buff, len);
				printk("Succesfully read:  %s\n",buff);
			}
			br_clanova--;
			pos_citanja++;
			if(pos_citanja==16) {pos_citanja=0;}

			//printk("citanje,upis %d, %d\n", pos_citanja, pos_upis);

			if(ret)
				return -EFAULT;
			printk(KERN_INFO "Succesfully read\n");
		}
		else{
			printk(KERN_WARNING "Fifo is empty\n");

		}


	//up(&sem);
	//wake_up_interruptible(&writeQ);
	endRead = endRead +1;
	return len;
}

ssize_t fifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
	char buff[BUFF_SIZE];
	int  value;
	int ret;
	char *p;
	char kopija[BUFF_SIZE];		
	p = buff;
	ret = copy_from_user(buff, buffer, length);
	if(ret)
		return -EFAULT;
	buff[length-1] = '\0';

	
	
	//if(down_interruptible(&sem))
	//	return -ERESTARTSYS;
	/*while(pos == 10)
	{
		up(&sem);
		if(wait_event_interruptible(writeQ,(pos<10)))
			return -ERESTARTSYS;
		if(down_interruptible(&sem))
			return -ERESTARTSYS;
	}*/
	strcpy(kopija,buff);
	if(length==6){
	kopija[strlen(kopija)-1]='\0';}
	else if(length==7) {
	kopija[strlen(kopija)-2]='\0';}


	if(!strcmp(buff, hex)) { h=1; }
	else if( !strcmp(buff, dec)) { h=0;}
	
	else if( !strcmp(num, kopija)) {
			if(length==6){
				n=buff[length-2]-48;}
			else if(length==7) {
				n=(buff[length-3]-48)*10 + (buff[length-2]-48);}
		}
	else{
		if( br_clanova < 16)
		{
			while(1){

				ret = sscanf(p,"%x,",&value);

				printk(KERN_INFO "Succesfully wrote value  %d ",value);
				fifo[pos_upis] = value;
				br_clanova ++;
				pos_upis = pos_upis + 1;
				if(pos_upis==16) {pos_upis=0; }
			while(p[0] != ',' && p[0] != '\0')
				++p ;
			if(*p == ',')
				++p;
			else
				break;
				}

			//printk("pos citanje, upis %d,%d\n", pos_citanja, pos_upis);
		}
		else
		{
		printk(KERN_WARNING "fifo is full\n"); 
		}
	}

	//up(&sem);
	//wake_up_interruptible(&readQ);

	return length;
}

static int __init fifo_init(void)
{
   int ret = 0;
	int i=0;
	
	//sema_init(&sem,1);

	//Initialize array
	for (i=0; i<10; i++)
		fifo[i] = 0;

   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "fifo");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "fifo_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "fifo");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

static void __exit fifo_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(fifo_init);
module_exit(fifo_exit);
