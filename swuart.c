/***************************************************************************
*    Copyright 2017 Mitel Networks
*    All Rights Reserved
*
****************************************************************************
*
****************************************************************************
*    Description:
*
* Software Uart for 6930_SIP  to hook i680 keyboard
*
****************************************************************************/
#include "cfg_global.h"
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/irq.h>

#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/broadcom/bcm_major.h>

#include <linux/gpio.h>

#include <linux/broadcom/user-gpio.h>
#include <linux/broadcom/bcmring/gpio_defs.h>
#include <mach/csp/gpiomux.h>

#include <linux/slab.h>

//For hrtimer
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#define RX_BUFFER_SIZE	256
unsigned char RX_BUFFER[RX_BUFFER_SIZE+1];

unsigned char RX_DATA=0;
struct swuartISRparams {
    struct hrtimer   timer;
    unsigned	   	irq_disabled;
    int			      irq;
};
/* ---- Private Variables ------------------------------------------------ */

static struct swuartISRparams swuart_timer;

static   ktime_t ktime_swuart_start; // 833us for start bit
static   ktime_t ktime_swuart_data; // 167us for 600bd rate
static int    swuart_irq_req = 0;

// configed data
int SOFTUART_TX = CONFIG_MITEL_TX_SOFT_UART;
int SOFTUART_RX = CONFIG_MITEL_RX_SOFT_UART;
int SOFTUART_BAUD_RATE = CONFIG_MITEL_SOFT_UART_BAUD_RATE;

#define SWUART_DRV_DEV_NAME   "swuart"

static  dev_t           swuartDevNum = MKDEV( BCM_SOFT_UART_MAJOR, 0 );
static  struct  cdev    swuartDrvCDev;

// SWUART Receive related
/*static int           swuartRxClock = 0;
static uint8_t       swuartRxData = 0;
static int           swuartRxBitIndex = 0;
static unsigned int  swuartClock = 0;
static uint8_t       swuartRxCodeComplete = 0; 
*/
static spinlock_t    swuart_lock;


// SWUART Send related

/* ---- Private Function Prototypes -------------------------------------- */
static irqreturn_t swuart_rx_isr(int irq, void *data);
static enum hrtimer_restart SWUARTTimerFunc( struct hrtimer *timer );
//static enum hrtimer_restart SWUARTTransmitTimerFunc( struct hrtimer *TxTimer );

//static unsigned int swuart_poll(struct file *, struct poll_table_struct *);
//static wait_queue_head_t swuart_wq_poll;


/* ---- Functions  ------------------------------------------------------- */


static irqreturn_t swuart_rx_isr (int irq, void *data) 
{
	int rx_val;
   struct swuartISRparams *swuartHandle = data;
   unsigned long    flags;
   int    SOFTUART_POLL_PERIOD_START = (1000000/SOFTUART_BAUD_RATE/2) * 1000;  // 833us for 600bdrate  
   printk(KERN_DEBUG " POLLING_START==%d \n",SOFTUART_POLL_PERIOD_START);

   rx_val = gpio_get_value( SOFTUART_RX );
  // printk(KERN_DEBUG "read RX valure == %d  %s \n", rx_val, __FUNCTION__);
   spin_lock_irqsave( &swuart_lock, flags);
   
  if (( rx_val == 0 ) && (!swuartHandle->irq_disabled))  // got start bit and interrupt enabled
   
   {
      // turn off RX interrupt 
      printk(KERN_DEBUG "Got falling edge and interrup ended \n");
      swuartHandle->irq_disabled = 1;
      disable_irq_nosync( irq );

   //  printk(KERN_DEBUG "handle->irq ==== %d \n", swuartHandle->irq);
    
    ktime_swuart_start = ktime_set( 0, SOFTUART_POLL_PERIOD_START );// sampling start bit 
    
;
     hrtimer_start( &swuartHandle->timer,ktime_swuart_start, HRTIMER_MODE_REL ); // 833us for 600 bd rate
     printk(KERN_DEBUG "Start timer and disalbed interrupt \n");
   }
   
   // clean up irq
   GpioHw_IrqClear( SOFTUART_RX );
   spin_unlock_irqrestore( &swuart_lock, flags);
   return IRQ_HANDLED;
}

static int swuart_open(struct inode *inode, struct file *filp)
{
   printk( KERN_DEBUG "SW UART  OPEN called=====" );
   return 0;
} 

static enum hrtimer_restart SWUARTTimerFunc( struct hrtimer *timer )
{
   static int bit=-1; 
   static int index;
	int rx_bit;
   int SOFTUART_POLL_PERIOD_DATA = (1000000/SOFTUART_BAUD_RATE) * 1000;  // 1667us for 600bdrate  
   
   ktime_swuart_data = ktime_set( 0, SOFTUART_POLL_PERIOD_DATA ); 
   rx_bit = gpio_get_value( SOFTUART_RX );
 //  printk(KERN_DEBUG "in timer read RX bit  == %d  %s \n", rx_bit, __FUNCTION__);
  // if( bit== -1 )   // start bit received
    // {
  // printk(KERN_DEBUG " recive....\n");
       bit++;
     // rx_bit = gpio_get_valure( SOFTUART_RX );

        // save the bit (including the start bit)
     SOFTUART_RX   |= rx_bit << bit;

   if(++bit  >  7 )
   {   
   // save data to buffer 
   RX_BUFFER[ index ] = SOFTUART_RX;

   index++; 
   printk(KERN_DEBUG "8 bit   done, rx buffer data[0-7] %x %x %x %x %x %x %x %x \n",   RX_BUFFER[0] , RX_BUFFER[1],
       RX_BUFFER[2], RX_BUFFER[3] , RX_BUFFER[4], RX_BUFFER[5], RX_BUFFER[6], RX_BUFFER[7]);

 // stop receiving
       bit = 0;
       SOFTUART_RX = 0;          
           
           // measurement cycle ended 
      swuart_timer.irq_disabled = 0;
    	enable_irq(117);
   //  enable_irq( swuart_timer.irq); 
     return HRTIMER_NORESTART;
   }  // END IF


 /*     else if( bit == 8 ) // stop bit take data as valide one  ==== 8 ??
      {
     
       bit= -1;

   RX_BUFFER[0] = SOFTUART_RX;
      printk(KERN_DEBUG "8 bit done, rx buffer data[0-7] %x %x %x %x %x %x %x %x \n",   RX_BUFFER[0] , RX_BUFFER[1],
   RX_BUFFER[2], RX_BUFFER[3] , RX_BUFFER[4], RX_BUFFER[5], RX_BUFFER[6], RX_BUFFER[7]);
   
      // Enable interrupt
      swuart_timer.irq_disabled = 0;
      enable_irq( 117 );
      return HRTIMER_NORESTART;

      }  //END IFF
*/
 //  }

  // printk(KERN_DEBUG "  should not be here....\n");
   hrtimer_forward_now(timer, ns_to_ktime(SOFTUART_POLL_PERIOD_DATA));
      // for next bit
      //    hrtimer_forward_now(timer, ktime_swuart_data);
  // return HRTIMER_NORESTART;
   return HRTIMER_RESTART;
}




static struct file_operations swuart_fops = 
{
   owner: THIS_MODULE,
   //read : swuart_read,
  //unlocked_ioctl:  sw_uart_ioctl,
  // write: swuart_write,
   open:  swuart_open, 
};



static int __init swuart_init(void)

{ 
   int ret;
   gpiomux_rc_e status;

   printk(KERN_EMERG "soft uart added in\n"); 

   printk( KERN_DEBUG "SW UART init called=====" );
 
   ret =register_chrdev_region( swuartDevNum, 1,  "bcmring_swuart") ; 

   cdev_init ( &swuartDrvCDev, &swuart_fops );
   swuartDrvCDev.owner  = THIS_MODULE;
   if ((ret = cdev_add( &swuartDrvCDev, swuartDevNum, 1 )) != 0 )
    {
      printk( KERN_WARNING "swuart driver: cdev_add failed: %d\n", ret );
      goto out_unregister;
    }

   printk( KERN_WARNING "swuart driver: cdev_add ok : %d\n", ret );

   if(ret <0) 
   {
   printk( KERN_ERR "SWUART: register_chrdev failed for major %u\n",BCM_SOFT_UART_MAJOR );
   return( -EFAULT );
   }
   printk(KERN_EMERG "SW UART init...line of %d\n " , __LINE__); 
   printk(KERN_EMERG "SW UART major = %d\n" ,BCM_SOFT_UART_MAJOR); 

  
  // Initialization for SOFTUART write operation : GPIO 20 output

   status = gpiomux_request( SOFTUART_TX, chipcHw_GPIO_FUNCTION_GPIO,"SOFTUART TX");
   if (status != gpiomux_rc_SUCCESS) {
      printk( KERN_DEBUG "%s %d  gpio TX request failurer %d\n", __FUNCTION__, __LINE__, status);
   }  else {
      gpio_direction_output( SOFTUART_TX, 1);
      // printk( KERN_DEBUG "request_irq succeeded. SWUART_RX: %d IRQ: %d\n",SOFTUART_TX, gpio_to_irq( SOFTUART_RX ));
   
      printk( KERN_DEBUG "%s %d  gpio TX request ok  %d\n", __FUNCTION__, __LINE__, status);
  } 
   
   // Set direction to input  GPIO 17
   
   status = gpiomux_request( SOFTUART_RX, chipcHw_GPIO_FUNCTION_GPIO, "SOFTUART RX");
   if (status != gpiomux_rc_SUCCESS) {
      printk( KERN_DEBUG "%s %d  gpio RX requtest failurer %d\n", __FUNCTION__, __LINE__, status);
   }
   gpio_direction_input( SOFTUART_RX );
   

/* Set direction to input */
//    gpio_direction_input( gpio );
   
    /* Enable interrupt */
  /*  GpioHw_IrqEnable( gpio );
    dhsg_timer.irq_disabled = 0;
    dhsg_timer.irq = gpio_to_irq( gpio );
*/


   // Enable interupt
   GpioHw_IrqEnable( SOFTUART_RX );
   swuart_timer.irq_disabled = 0;
   swuart_timer.irq = gpio_to_irq( SOFTUART_RX );
 
   // create a timer for soft uart
    
   hrtimer_init( &swuart_timer.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
   // printk( KERN_DEBUG "hrtimer_init done..\n" );
   swuart_timer.timer.function = &SWUARTTimerFunc;
  //  printk( KERN_DEBUG "TimerFunc added to hrtimer..\n" );
    

    // Request IRQ
    if (request_irq( gpio_to_irq( SOFTUART_RX ), swuart_rx_isr, IRQF_TRIGGER_FALLING, "SOFTUART Recevie " , &swuart_timer) < 0)
   {
   printk(KERN_ERR "SW-UART: request_irq %u failed\n", gpio_to_irq( SOFTUART_RX ));
   }
  else 
   {
   printk( KERN_DEBUG "request_irq succeeded. gpio: %d IRQ: %d\n",SOFTUART_RX, gpio_to_irq( SOFTUART_RX ) );
   swuart_irq_req = 1;
   }
   goto done; 
   
  

out_unregister:

      unregister_chrdev_region( swuartDevNum, 1);
done:
     return ret;
}
static void __exit swuart_exit(void)
{
   
   printk(KERN_EMERG "Soft uart removed..\n"); 
   unregister_chrdev_region( swuartDevNum, 1);
  // GpioHw_IrqClear (SOFTUART_RX );
   // for debug and test purpose
  // gpio_free( SOFTUART_RX);
  // gpio_free( SOFTUART_TX);
}


module_init( swuart_init );

module_exit( swuart_exit );


MODULE_AUTHOR("Mitel");
MODULE_DESCRIPTION( "Mitel SWUART driver" );
MODULE_LICENSE( "GPL" );
