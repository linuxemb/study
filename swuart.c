#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/irq.h>
#include <linux/semaphore.h>

#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/broadcom/bcm_major.h>
#include <linux/vmalloc.h>             /* For memory alloc */

#include <linux/gpio.h>

#include <linux/broadcom/user-gpio.h>
#include <linux/broadcom/bcmring/gpio_defs.h>
#include <mach/csp/gpiomux.h>

#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/irqflags.h>

//For hrtimer related
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#define RX_BUFFER_SIZE 256	



/* ---- ---Private Variables ------------------------------------------------ */
// interrupt related
struct swuartISRparams {
    struct hrtimer   suartRxtimer;
    unsigned	   	irqDisabled;
    int			      irq;
};

struct swuartStructD {
   struct hrtimer    suRxtimer;
   struct hrtimer    suRxGuard;
   unsigned	   	   irqDisabled;
   int			      suartIrq;
   int			      suartConfigB;
	ktime_t           KTime_RxStart;
	ktime_t           KTime_Rxsuart;
	ktime_t           KTime_RxGuard;
	ktime_t           KTime_Txsuart;
}; 
static struct swuartStructD swuartData;

// timer related
static struct swuartISRparams swuart_timer;
static struct hrtimer swuartTxTimer;  // timer for transmit
// interrupt related
static int    swUartIrqReq = 0;
static char   swUartBanner[] __initdata = KERN_INFO " swuart Driver: 1.00\n";

// counter related tx, rx   
static int bit = -1;
static int txbit = -1;
// configed data
int softUartTx = CONFIG_MITEL_TX_SOFT_UART;
int softUartRx = CONFIG_MITEL_RX_SOFT_UART;
int softUartBdRate = CONFIG_MITEL_SOFT_UART_BAUD_RATE;

static  dev_t           swuartDevNum = MKDEV( BCM_SOFT_UART_MAJOR, 0 );
static  struct  cdev    swuartDrvCDev;

// SWUART Receive related
static uint8_t       swuartRxData = 0;
static bool          halfPeriod = true;
unsigned char        rxBuffer[ RX_BUFFER_SIZE ];
unsigned char        txBuffer[ RX_BUFFER_SIZE ];
int                  txBufferHead = 0;
int                  txBufferTail = 0;

int                  swuartTail = 0;
int                  swuartHead = 0;
uint8_t              full = 0;
uint8_t              ready = 0;

uint8_t             swuartRxCodeComplete =0;

static spinlock_t   swuartLock ;

// SWUART Send related
static uint8_t       swuartTxData = 0; 

/* ---- Private Function Prototypes -------------------------------------- */
static irqreturn_t swuart_rx_isr(int irq, void *data);
static enum hrtimer_restart swuartRxTimeFunc( struct hrtimer *Rxtimer );
//static enum hrtimer_restart swuartRxGuardTimeFunc( struct hrtimer *RxGtimer );
static enum hrtimer_restart swuartTxTimeFunc( struct hrtimer *TxTimer );
static unsigned int swuart_poll(struct file *, struct poll_table_struct *dWait);
static wait_queue_head_t swuart_wq_poll;
static int swuartTransmitByte ( uint8_t );

/* ---- Functions  ------------------------------------------------------- */

static   ssize_t   swuart_write(struct file *swuart_filp, 
                              const char __user *swuart_buff, 
                              size_t swuart_count, 
                              loff_t *offp)
{
   uint8_t                *kbuf;
   uint8_t                lswuartWrtB[50];
   size_t                 lSwuartWrtBs = 0;
   int                    bytes_written;
   int                    bufsize;
   int                    rc;
   int                    i;
//   unsigned long          flags;

   if ( (swuart_count == 0) || ( swuart_count > 200) ) {
      return -EINVAL;
   }
   bufsize = swuart_count;
   lSwuartWrtBs = sizeof( lswuartWrtB);
   kbuf = &lswuartWrtB[0];
   if ( swuart_count >= lSwuartWrtBs) {
      kbuf = vmalloc( bufsize );
      if ( !kbuf )
      {
         return -ENOMEM;
      }
   }

   if (( rc = copy_from_user( kbuf, swuart_buff, swuart_count )) < 0 )
   {
      printk( KERN_ERR "%s: failed to copy data from user swuart_buff\n", __FUNCTION__ );
      if ( kbuf != &lswuartWrtB[0]) {
         vfree( kbuf );
      }
      return -EIO;
   }
//   spin_lock_irqsave( &swuartTxLock, flags);
   if ( txBufferHead == txBufferTail) {
      swuartTransmitByte( kbuf[0]);
      txBufferTail++;
      bufsize--;
   }
   for ( i = 0; i < bufsize; i++) {
      txBuffer[ txBufferTail++] = kbuf[i]; 
   }
   if ( kbuf != &lswuartWrtB[0] ) {
      vfree( kbuf );
   }
   bytes_written = swuart_count;
//   spin_unlock_irqrestore( &swuartTxLock, flags);
   return bytes_written;
}

static irqreturn_t swuart_rx_isr (int irq, void *data) 
{
   int rxVal;
   struct swuartISRparams *swuartHandle = data;
   
   halfPeriod = true; // hope to get into 0.5 period 
   rxVal = gpio_get_value( softUartRx );
   
   // got start bit and interrupt enabled
   if (( rxVal == 0 ) && (!swuartHandle->irqDisabled))  
   {
      // turn off RX interrupt 
      //printk(KERN_DEBUG "Got falling edge and interrup ended \n");
      swuartHandle->irqDisabled = 1;
      disable_irq_nosync( irq );

      if ( hrtimer_callback_running( &swuartHandle->suartRxtimer) == 0 ) {
         hrtimer_start( &swuartHandle->suartRxtimer, swuartData.KTime_RxStart, HRTIMER_MODE_REL ); 
      } else {
         printk(KERN_ERR "SW-UART: %d  overrun \n", __LINE__);
      }
   }
   
   GpioHw_IrqClear( softUartRx );
   return IRQ_HANDLED;
}
static int swuart_open(struct inode *inode, struct file *filp)
{
   if ( inode == NULL) {
      return -EINVAL;
   }
   if ( filp == NULL) {
      return -EINVAL;
   }
   if ( swUartIrqReq == 0 ) {
      printk( KERN_WARNING "swuart: not init\n" );
      return -EFAULT;
   }
   //printk( KERN_DEBUG "SW UART  OPEN called" );
   return 0;
} 

/*  Sending one byte data                     */
static int swuartTransmitByte ( uint8_t txData )
{
   swuartTxData =  txData; 
   hrtimer_start( &swuartTxTimer, ktime_set( 0, 1  ), HRTIMER_MODE_REL );
   // printk( KERN_WARNING "Transmit one bytdata.    \n" );
   return 0;
}

static   ssize_t   swuart_read(struct file *filp, 
                             char __user *buff, 
                             size_t count, 
                             loff_t *offp)
{
   uint8_t swuartOutput = 0;
   unsigned long          flags;
   if (( swuartTail == swuartHead) && !full )
   {
    printk( KERN_WARNING "Ety receiveing buffer    \n" );
      return 0;
   }
   if ( count != 1 ) {
      printk( KERN_ERR "%s %d: not supported count %d\n", __FUNCTION__, __LINE__, count);
      return -EINVAL;
   }
   // read rx buffer and cp to user
   swuartOutput = rxBuffer[swuartTail++];
   if ( copy_to_user( &buff[0], &swuartOutput, sizeof(uint8_t)) != 0 )
   {
        return( -EFAULT );
   }
      spin_lock_irqsave(&swuartLock, flags);
   //   swuartRxCodeComplete = 0;
      //swuartTail ++;
          if( swuartTail >= 256 )
          {
             swuartTail = 0;
             full = 0;
             ready = 0;
             printk( KERN_DEBUG "SW UART  RX buffer wraped \n");
          }
      spin_unlock_irqrestore(&swuartLock, flags);

  
   /*
   if ( swuartHead == swuartTail) {
      swuartTail = 0;
      swuartHead = 0;
   }   */


   return sizeof(uint8_t);
}



static unsigned int swuart_poll(struct file *dFile, struct poll_table_struct *dWait)
{
   unsigned int mask = 0;
   poll_wait(dFile,&swuart_wq_poll,dWait);
   if ( swuartTail != swuartHead) {
      mask |= (POLLIN | POLLRDNORM ) ;
   }
  return mask;
}




static enum hrtimer_restart swuartRxTimeFunc( struct hrtimer *Rxtimer )
{  
	int rxBit;
   unsigned long  flags;
   /*
    *  Need verify is 0.5 period time up or 1 time period time up: if in half timeup, 
    *  need validt the start bit existance.
    *  else need sampling 9 bits. and validate the stop bits afterwards a
    *  with 8bits data,   no parity check, it's 10 bit periods
    */
   if (halfPeriod) 
   {
      /* 
         validate start bit and stop interrupt and start 1 period timer; 
         set half period flag false
      */
      halfPeriod = false;
      rxBit = gpio_get_value( softUartRx );
      if ( 0== rxBit  &&  -1 == bit )
      {
         swuart_timer.irqDisabled = 1;
         bit ++;
         hrtimer_forward_now(Rxtimer, swuartData.KTime_Rxsuart);
         return HRTIMER_RESTART;
      }
      else
      {
         /* no start bit received, enable irq, go back to wait for start bit */
//         printk( KERN_WARNING " Not a start bit recvd \n" );
         enable_irq( swuart_timer.irq); 
         swuart_timer.irqDisabled = 0;
         bit = -1;
         return HRTIMER_NORESTART;
      }
   } 
   else
   { 
      /*             measuring                               */
      if (bit < 8 && bit >= 0 ) // in data bit
      {
         rxBit = gpio_get_value( softUartRx );
         swuartRxData   |= rxBit << bit;
         bit++;
         hrtimer_forward_now(Rxtimer, swuartData.KTime_Rxsuart);
         return HRTIMER_RESTART;
      }
      else if (bit == 8) // stop bit
      {  
            swuartRxCodeComplete = swuartRxData ;
            printk( KERN_WARNING "swuart_Recved_data=0x%x \n" , swuartRxData >> 1 );
            swuartRxData = 0;
         // If received valid stop bit, Then buffer data
         if (gpio_get_value( softUartRx ))
         { 
            if(( swuartTail == swuartHead ) && full )
             printk( KERN_DEBUG "SW UART  RX buffer over run  \n");
           rxBuffer[ swuartHead ]   = swuartRxCodeComplete;
             swuartHead++;
            spin_lock_irqsave( &swuartLock, flags);
               if (swuartHead == 256 )
               full = 1;

               ready =1;
               wake_up_interruptible( &swuart_wq_poll );
            spin_unlock_irqrestore( &swuartLock, flags); 
         }
        }
        
        else 
          printk( KERN_WARNING " Stop bit invalid \n" );
         
            spin_lock_irqsave( &swuartLock, flags);
         swuartRxData = 0;
         // endbit8 measurment ended 
         enable_irq( swuart_timer.irq); 
         swuart_timer.irqDisabled = 0;
         bit = -1;
        spin_unlock_irqrestore( &swuartLock, flags); 
         return HRTIMER_NORESTART;
      
   } //end 1 period
         return HRTIMER_NORESTART;
}

#if 0
static enum hrtimer_restart swuartRxGuardTimeFunc( struct hrtimer *RxGtimer )
{  
   unsigned     ll01;
	struct swuartStructD  *ctx = container_of(RxGtimer, struct swuartStructD, suRxGuard);
   
   ll01 = ctx->irqDisabled;
   if ( bit == -1 ) {
      swuartRxData = 0;
      // endbit8 measurment ended 
      enable_irq( swuart_timer.irq); 
      swuart_timer.irqDisabled = 0;
      return HRTIMER_NORESTART;
   }
   if (gpio_get_value( softUartRx ))
   {             
      rxBuffer[swuartTail++] = swuartRxData;
      wake_up_interruptible( &swuart_wq_poll );
   }
   else 
   {
      printk( KERN_WARNING "%s %d  Stop bit invalid \n", __FUNCTION__, __LINE__ );
   }
   swuartRxData = 0;
   // endbit8 measurment ended 
   enable_irq( swuart_timer.irq); 
   swuart_timer.irqDisabled = 0;
   bit = -1;
   return HRTIMER_NORESTART;
}
#endif

static enum hrtimer_restart swuartTxTimeFunc( struct hrtimer * TxTimer )
{    
   enum hrtimer_restart    ltxRtn = HRTIMER_NORESTART;
  
//   printk( KERN_WARNING " TX timer  txbit= 0x%x swuartTxData 0x%02x\n", txbit, swuartTxData );
//   printk( KERN_WARNING " TX timer  txBufferHead %d txBufferTail %d \n", txBufferHead, txBufferTail );

   if(txbit < 8 && txbit >= -1)
   {
      if(-1 == txbit ) // start bit
      {
      gpio_set_value( softUartTx, 0 );
      txbit++;
      }
      else if( 0 <= txbit && txbit <= 7 ) // data bit
      { 
         if (swuartTxData&0x1)
         {
            gpio_set_value( softUartTx, 1);
         }
         else 
         {
            gpio_set_value( softUartTx, 0);
         }
         // move down to the next bit
         txbit++;        
         swuartTxData = swuartTxData >> 1;
      }  else {
         printk( KERN_ERR "SWUART:%d logic error \n", __LINE__);
      } 
   }
   else if ( 8 == txbit )
   {
      gpio_set_value( softUartTx, 1);
      // printk( KERN_WARNING " sending stop bit  \n" );
      txbit = -1 ;
      if ( ++txBufferHead != txBufferTail) {
         ltxRtn = HRTIMER_RESTART;
         swuartTxData = txBuffer[ txBufferHead]; 
         hrtimer_forward_now( &swuartTxTimer, swuartData.KTime_Txsuart); // send data at 1 cycle period
//printk( KERN_WARNING " TX timer  txbit= 0x%x swuartTxData 0x%02x\n", txbit, swuartTxData );
//printk( KERN_WARNING " TX timer  txBufferHead %d txBufferTail %d \n", txBufferHead, txBufferTail );
      }
      if ( txBufferHead == txBufferTail) {
         txBufferHead = 0;
         txBufferTail = 0;
      }
      return ltxRtn;
   }  else {
      printk( KERN_ERR "SWUART:%d logic error \n", __LINE__);
   }   
     
   hrtimer_forward_now( &swuartTxTimer, swuartData.KTime_Txsuart); // send data at 1 cycle period
   ltxRtn = HRTIMER_RESTART;
   return ltxRtn;

}

static struct file_operations swuart_fops = 
{
   owner: THIS_MODULE,
   read : swuart_read,
   poll:  swuart_poll,
   open:  swuart_open,
   write: swuart_write,
 
};

static int __init swuart_init(void)

{
   int               ret;
   int               sioRsl;
   int               lsbaudt;
   gpiomux_rc_e status;
   txbit=-1;
   lsbaudt = -1;
   ret = register_chrdev_region( swuartDevNum, 1,  "bcmring_swuart") ; 

   if (ret  < 0 ) {
      printk( KERN_ERR "SWUART: register_chrdev_region failed \n");
      return ret;
   } 
   

   cdev_init ( &swuartDrvCDev, &swuart_fops );
   swuartDrvCDev.owner  = THIS_MODULE;
   if ((ret = cdev_add( &swuartDrvCDev, swuartDevNum, 1 )) != 0 )
    {
      printk( KERN_WARNING "swuart driver: cdev_add failed: %d\n", ret );
      goto out_unregister;
    }

   if(ret <0) 
   {
      printk( KERN_ERR "SWUART: register_chrdev failed for major %u\n",BCM_SOFT_UART_MAJOR );
      return( -EFAULT );
   }
  
   // Initialization for SOFTUART write operation : GPIO 20 output
   status = gpiomux_request( softUartTx, chipcHw_GPIO_FUNCTION_GPIO,"SOFTUART TX");
   if (status != gpiomux_rc_SUCCESS) 
   {
      printk( KERN_DEBUG "%s %d  gpio TX request failurer %d\n", __FUNCTION__, __LINE__, status);
   }  else {
      gpio_direction_output( softUartTx, 1);
   //  printk( KERN_DEBUG "%s %d  gpio TX request ok  %d\n", __FUNCTION__, __LINE__, status);
   } 
   
   // Set direction to input  GPIO 17
   status = gpiomux_request( softUartRx, chipcHw_GPIO_FUNCTION_GPIO, "SOFTUART RX");
   if (status != gpiomux_rc_SUCCESS) 
   {
      printk( KERN_DEBUG "%s %d  gpio RX requtest failurer %d\n", __FUNCTION__, __LINE__, status);
      ret = -EFAULT;
      goto out_unregister;
   }
   gpio_direction_input( softUartRx );

   // Enable interupt
   GpioHw_IrqEnable( softUartRx );
   swuart_timer.irqDisabled = 0;
   swuart_timer.irq = gpio_to_irq( softUartRx );
 
   swuartData.suartConfigB = CONFIG_MITEL_SOFT_UART_BAUD_RATE;
   /* create a Receive timer for soft uart      */
   hrtimer_init( &swuart_timer.suartRxtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
   swuart_timer.suartRxtimer.function = &swuartRxTimeFunc;
   lsbaudt = (1000000/swuartData.suartConfigB) * 1000;  // 1667us for 600bdrate  
   swuartData.KTime_Rxsuart = ktime_set( 0, lsbaudt ); // send data at 1 cycle period
   lsbaudt = (1000000/swuartData.suartConfigB/2) * 1000; // 833us for 600bdrate  
   swuartData.KTime_RxStart = ktime_set( 0, lsbaudt ); // send data at 1 cycle period

#if 0
   /*    soft uart rx guard, per character      */
   hrtimer_init( &swuartData.suRxGuard, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
   swuartData.suRxGuard.function = &swuartRxGuardTimeFunc;

   lsbaudt = (1000000/swuartData.suartConfigB/2) * 1000; // 833us for 600bdrate  
   lsbaudt *= 19;
   lsbaudt *= 92;
   lsbaudt /= 100;
   swuartData.KTime_RxGuard = ktime_set( 0, lsbaudt ); 
#endif
   /* Create a Transmit timer for soft uart    */
   hrtimer_init( &swuartTxTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
   swuartTxTimer.function = &swuartTxTimeFunc;
   lsbaudt = (1000000/swuartData.suartConfigB) * 1000; 
   swuartData.KTime_Txsuart = ktime_set( 0, lsbaudt ); // send data at 1 cycle period

   // Request IRQ
   sioRsl = request_irq( gpio_to_irq( softUartRx ), swuart_rx_isr, IRQF_TRIGGER_FALLING, "SOFTUART Receive " , &swuart_timer);
   if ( sioRsl < 0 ) 
   {
      printk(KERN_ERR "SW-UART: request_irq %u failed\n", gpio_to_irq( softUartRx ));
      ret = -EFAULT;
      goto out_unregister;
   }
   else 
   {
      printk( KERN_DEBUG "request_irq succeeded. gpio: %d IRQ: %d\n",softUartRx, gpio_to_irq( softUartRx ) );
      swUartIrqReq = 1;
   }
   // wait queue initial
   init_waitqueue_head(&swuart_wq_poll);

   printk( swUartBanner );
   goto done; 

out_unregister:
      unregister_chrdev_region( swuartDevNum, 1);
done:
     return ret;
}



static void __exit swuart_exit(void)
{
   free_irq( swuart_timer.irq, &swuart_timer );
   hrtimer_cancel( &swuart_timer.suartRxtimer );
   hrtimer_cancel( &swuartTxTimer );
   cdev_del( &swuartDrvCDev );  
   printk(KERN_WARNING "Soft uart removed.\n"); 
   unregister_chrdev_region( swuartDevNum, 1);
}


module_init( swuart_init );
module_exit( swuart_exit );

MODULE_AUTHOR("my");
MODULE_DESCRIPTION( "my SWUART driver" );
MODULE_LICENSE( "GPL" );



/*------------------------------------------------------------------------------------------------------
 *                FSM for soft uart   State :
 *                unknown->wait_for_start_bit-> waitfor8bits data->waitfor stop bit->wait_for_start_bit
 *                                
 *  -------------------------------------------------------------------------------------------------------
 *  unknown state
 *       |  module init ok
 *       V
 *     waiting-for-start-bit <--+-------------|
 *       |  falling edge        |             |
 *       V                     fail 	       |
 *     validate startbit   -->--+-------------|
 *        |                                   |
 *       |<------------  +                    ^
 *       V               |                    |
 *      collect 8-bit ->+-		                |
 *       |                                    |
 *       V                      fail          |
 *    waiting for stop bit --> go back--- >---|
 *       |                     waiting for start bit
 *       |                                    |
 *       V                                    |
 *      buffer character		                |	
 *       |				                         |	
 *       V				                         |		
 *     go back to waiting for--------->- --- -+
 *     start bit
 *   
 */


//---------------------------------------------------------------------------------------------

//     Transmit one byte State:  idle--> send start--> send data-> send stop -> idle
//------------------------------------- -------------------------------------------------------
/*     
 *        
 *      Verfiy if TxBuffer is not null                     
 *           |
 *      send start bit, start TxTimer disable intrupt
 *           |
 *           V----------<-------|
 *       waitfor time out ----- +      
 *           |  
 *           V
 *       shift TxBuffer 1 bit -------<-- --| 
 *           |   
 *       write to TX pin, start TxTimer    |  
 *           |                             |  
 *           | --------- < ------ -|       |  
 *       wait for time out---------+       |  
 *           |                             |  
 *           V                             |
 *       8 bit done ?----->----No----------+
 *          Y|
 *           V      
 *      send stop bit, start TxTimer
 *           |
 *           V
 *      Transmit data cycle complete reset Tx counter,stop timer        
             |
             V
         Enable interrupt  
 *
 */

