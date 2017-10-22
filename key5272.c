#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/time.h>
#include "keydev.h"

 int base = 0x10000000;
 unsigned long volatile  key_head;
 volatile unsigned long  key_tail;
//static  struct work_struct  *key_wq;

// DECLARE_WAIT_QUEIE_HEAD(short_queue);

static int key_major = KEYDEV_MAJOR;

module_param(key_major, int, S_IRUGO);

struct key_dev *key_devp; 

struct cdev cdev; 
unsigned long  kit, polling_delay;
void workq_fn (unsigned long arg)
{
  printk(KERN_INFO "in work_FNa");
}
DECLARE_WORK( workq,workq_fn);
int sched_workq(char *buf, char **start, off_t offset, int len, int *eof, void *arg)

{
   printk(KERN_INFO "infsched_wq");
   schedule_work(&workq);

}   

/*i===========================*/
void do_key_read()
{
   char key_val = 0; 
   unsigned long gpio_addr =  key_base + 0x96;
	key_val = ioread8( gpio_addr);
       if (key_val != 0xff)   /*key pressed maybe */  
	{       udelay(kit);    /* remove shaking  */
         if(key_val == ioread8( gpio_addr))   /* key still been pressed  */
             {           head   = key_val;                /*take key_Val copy to ring buffer  */
                          head++;
             }
         /* start timer  */

       dev->polling_timer.expires = jiffies + polling_delay * HZ;
       add_timer (&dev->polling_timer);
	return 0;
}

 
// timer handler()

polling_timer_handler(unsigned long parameter)

{
  printk ( "polling again after %u \n", parameter);

  /* now I need read in my gpio data of port c  */

  do_key_read();

}

/*   open======================================*/

int key_open(struct inode *inode, struct file *filp)
{
    struct key_dev *dev;
    
    int num = MINOR(inode->i_rdev);

    dev = &key_devp;
    
    filp->private_data = dev;
/*    
//init ring buffer
	  dev-> tail  = dev->head = 0;
 
 //  init waitqueue
	 init_waitqueue_head(&dev->wq);
*/ 
//init timer & parameters
 
 polling_delay = POLLING_DELAY;
 kit          =  KIT;
 
 init_timer(&dev->polling_timer);
 dev->polling_timer.expires = jiffies + polling_delay * HZ;
 dev->polling_timer.data =  polling_delay;
 dev->polling_timer.function = polling_timer_handler;
 add_timer (&dev->polling_timer);

    return 0; 
}

/*   read================================================================*/


static ssize_t key_read(struct file *filp, char __user *buf, size_t size,loff_t *ppos)
{

     return 0;
}

/*    write() ========================================================*/

static ssize_t key_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
      return 0;
}


int key_read_procmem (char *buf, char **start, off_t offse, int count, int *eof, void *data)
{	
	int i, j, len=0;
	struct key_dev *d= &key_devp;
	len+= sprintf(buf+len, "keydev:"); 
	printk (KERN_INFO  "key_reaad_procmem called");
        return len;     
}

/*    ioctl()==========================================================*/

int key_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{

	int SUCESS=0;
	switch (cmd)
		 {

		case KEY_IOCSETKIT:
		kit= arg;
		break;
		
		case KEY_IOCSETPOLLING:
		polling_delay = arg;
		break;
		default:
		return -EINVAL;
		} 
		return SUCESS;
}
				


int key_release(struct inode *inode, struct file *filp)
{
  return 0;
}

static const struct file_operations key_fops =
{
  .owner = THIS_MODULE,
  .open = key_open,
  .read = key_read,
  .write = key_write,
  .unlocked_ioctl = key_ioctl,

  .release = key_release,

};

/*   init()  ========================================*/

static int keydev_init(void)
{
  int result;
  int i;
 int key_base;

  dev_t devno = MKDEV(key_major, 0);
  key_base = base;
 create_proc_read_entry("schde_workq",0,NULL, *sched_workq, NULL);
 create_proc_read_entry("keydev", 0, NULL, *key_read_procmem, NULL); 
  if (key_major)
    result = register_chrdev_region(devno, 2, "keydev");
  else  
  {
    result = alloc_chrdev_region(&devno, 0, 2, "keydev");
    key_major = MAJOR(devno);
  }  
  
  if (result  <  0  )
  return result;

  cdev_init(&cdev, &key_fops);
  cdev.owner = THIS_MODULE;
  cdev.ops = &key_fops;
 l 
  cdev_add(&cdev, MKDEV(key_major, 0),1);
  key_devp = kmalloc(sizeof(struct key_dev), GFP_KERNEL);
  if (!key_devp)    
  {
    result =  - ENOMEM;
    goto fail_malloc;
  } 

  memset(key_devp, 0, sizeof(struct key_dev));

//init ring buffer
  key_tail  = key_head = key_devp;


// memory map for using readb
  key_base = (unsigned long) ioremap(key_base, 8);

 
//init work queue

//INIT_WORK( &key_wq, key_do_work);
 
 //init timer & parameters  in open()
 
  return 0;


fail_malloc:
     {unregister_chrdev_region(devno, 1);
  
     return result;
     }
}
static void keydev_exit(void)
{	  kfree(key_devp);  
	  unregister_chrdev_region(MKDEV(key_major, 0), 2); 

}


MODULE_AUTHOR("mto");
MODULE_LICENSE("GPL");

module_init(keydev_init);
module_exit(keydev_exit);
