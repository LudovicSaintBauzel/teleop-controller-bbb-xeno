#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/interrupt.h>

#include <rtdm/rtdm.h>
#include <native/pipe.h>
#include <native/timer.h>
#include <native/task.h>
#include <native/sem.h>
#include <native/mutex.h>
#include <native/heap.h>
#include <native/buffer.h>
#include <native/alarm.h>

#include <errno.h>
#include <stdio.h>

//Xenomai
#include <sys/mman.h>
#include <unistd.h>
#include <rtdm/rtdm_driver.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lucas Roche");

// Clock Module Peripheral Registers
#define CM_PER_START_ADDR 0x44e00000
#define CM_PER_LEN (0x44e03fff-CM_PER_START_ADDR)
// GPIO Registers
#define GPIO0_START_ADDR 0x44e07000
#define GPIO1_START_ADDR 0x4804C000
#define GPIO2_START_ADDR 0x481AC000
#define GPIO3_START_ADDR 0x481AE000
#define GPIO_LEN (0x481AEfff-GPIO0_START_ADDR)
#define GPIO_IRQSTATUS_SET_0 0x34
#define GPIO_RISINGDETECT 0x148
#define GPIO_FALLINGDETECT 0x14c

static unsigned int pin_chan_a = 30; // P9_11 - GPIO_30
static unsigned int pin_chan_b = 31; // P9_13 - GPIO_31
#define GPIO_MASK (1<<30)|(1<<31) // GPIO0_30

static unsigned int pin_pwm1 = 15; //P9_22 - GPIO_15
int temp = 500000;
int PWM_PERIOD = 1000000;
int PWM1_DUTY = 500000;
long temp_t = 0;

rtdm_task_t PWM1;
rtdm_timer_t PWM1_timer;
//RT_TASK PWM2;
//RT_ALARM PWM_alarm;

static int encoder_value = 0;

static volatile int chan_a_current_value = 0;
static volatile int chan_b_current_value = 0;


rtdm_irq_t irq_handle_a;
rtdm_irq_t irq_handle_b;


static int irq_handler_a(rtdm_irq_t* irq_handle);
static int irq_handler_b(rtdm_irq_t* irq_handle);


static int encoder_open(struct rtdm_dev_context* context, rtdm_user_info_t* user_info, int oflags) {
	int result = 0;
	void __iomem* mem = 0;
	int regval = 0;

	rtdm_printk(KERN_INFO "Initializing encoder LKM\n");

	if(!gpio_is_valid(pin_chan_a)) {
		rtdm_printk(KERN_INFO "Invalid channel A gpio\n");
		return -ENODEV;
	}

	if(!gpio_is_valid(pin_chan_b)) {
		rtdm_printk(KERN_INFO "Invalid channel B gpio\n");
		return -ENODEV;
	}
	
	if((result = gpio_request(pin_chan_a,"sysfs")) != 0) {
		rtdm_printk(KERN_ERR "gpio_request error\n");
		return -result;
	}
	gpio_direction_input(pin_chan_a);
	gpio_export(pin_chan_a,false);
	

	if((result = gpio_request(pin_chan_b,"sysfs")) != 0) {
		rtdm_printk(KERN_ERR "gpio_request error\n");
		return -result;
	}
	gpio_direction_input(pin_chan_b);
	gpio_export(pin_chan_b,false);


	
	rtdm_printk(KERN_INFO "GPIO2 init done\n");
	
	// Register config -- start
	mem = ioremap(GPIO0_START_ADDR, GPIO_LEN);
	if(!mem) {
		printk(KERN_ERR "ioremap error %d\n",errno);
		return -errno;
	}

	iowrite32(GPIO_MASK,mem+GPIO_IRQSTATUS_SET_0);
	regval = ioread32(mem+GPIO_RISINGDETECT);
	regval |= GPIO_MASK;
	iowrite32(regval,mem+GPIO_RISINGDETECT);
	regval = ioread32(mem+GPIO_FALLINGDETECT);
	regval |= GPIO_MASK;
	iowrite32(regval,mem+GPIO_FALLINGDETECT);
	iounmap(mem);
	// Register config -- end

	rtdm_printk(KERN_INFO "request_irq A2 : %d\n",result);
	result = rtdm_irq_request(&irq_handle_a,gpio_to_irq(pin_chan_a),(rtdm_irq_handler_t)irq_handler_a,0,"Channel A IRQ handler",NULL);
	rtdm_printk(KERN_INFO "rtdm_irq_request A2 : %d\n",result);


	rtdm_printk(KERN_INFO "request_irq B : %d\n",result);
	result = rtdm_irq_request(&irq_handle_b,gpio_to_irq(pin_chan_b),(rtdm_irq_handler_t)irq_handler_b,0,"Channel B IRQ handler",NULL);
	rtdm_printk(KERN_INFO "rtdm_irq_request B : %d\n",result);


	return result;
}

static int encoder_close(struct rtdm_dev_context* context,rtdm_user_info_t* user_info) {
	rtdm_printk(KERN_INFO "Exiting encoder LKM, encoder value is %d\n",encoder_value);

	rtdm_irq_free(&irq_handle_a);
	rtdm_irq_free(&irq_handle_b);
		
	gpio_unexport(pin_chan_a);
	gpio_unexport(pin_chan_b);
	
	gpio_free(pin_chan_a);
	gpio_free(pin_chan_b);

	rtdm_printk(KERN_INFO "End of encoder LKM\n");

	return 0;
}

static int irq_handler_a(rtdm_irq_t* irq_handle)
{
	chan_a_current_value = gpio_get_value(pin_chan_a);
	chan_b_current_value = gpio_get_value(pin_chan_b);
	if(chan_a_current_value) {
		if(chan_b_current_value) {
			--encoder_value;
		}
		else {
			++encoder_value;
		}
	}
	else {
		if(chan_b_current_value) {
			++encoder_value;
		}
		else {
			--encoder_value;
		}
	}
	return RTDM_IRQ_HANDLED;
} 


static int irq_handler_b(rtdm_irq_t* irq_handle)
{
	chan_b_current_value = gpio_get_value(pin_chan_b);
	if(chan_b_current_value) {
		if(chan_a_current_value) {
			++encoder_value;
		}
		else {
			--encoder_value;
		}
	}
	else {
		if(chan_a_current_value) {
			--encoder_value;
		}
		else {
			++encoder_value;
		}
	}
	return RTDM_IRQ_HANDLED;
}




void PWM1_timer_handler(rtdm_timer_t *PWM1_timer)
{
	//rtdm_printk(KERN_INFO "timer handler exec\n");
	gpio_set_value(pin_pwm1, 0);
}

void PWM1_thread(void *arg)
{
	rtdm_printk(KERN_INFO "Starting PWM thread 1...\n");
	
	rtdm_timer_init(&PWM1_timer, (rtdm_timer_handler_t) PWM1_timer_handler, "PWMtimer1");
	
	for(;;)
	{
		gpio_set_value(pin_pwm1, 1);
		rtdm_timer_start(&PWM1_timer, (nanosecs_abs_t) PWM1_DUTY, 0,  RTDM_TIMERMODE_RELATIVE );

		
		if(rtdm_task_wait_period() != 0)
			rtdm_printk(KERN_INFO "Task Wait Period Error 1.... \n");
	}
}


static int encoder_read_rt(struct rtdm_dev_context *context, rtdm_user_info_t * user_info, void *buf,size_t nbyte) {
	//rtdm_printk(KERN_INFO "encoder_read_rt: encoder value = %d\n",encoder_value);
	rtdm_safe_copy_to_user(user_info,buf,(void*)&encoder_value,nbyte);	
	return sizeof(int);
}

static int motor_write(struct rtdm_dev_context *context, rtdm_user_info_t *user_info, void *buf, size_t nbyte)
{
	rtdm_safe_copy_from_user(user_info, (void*)&temp, buf, nbyte);
	if(temp == 59595959)
	{
		encoder_value = 0;
		rtdm_printk(KERN_INFO "Data received from user 1: reset\n");		
	}
	else
	{
		PWM1_DUTY = temp;
		//rtdm_printk(KERN_INFO "Data received from user 1: temp: %i, PWMDUTY : %i\n", temp, PWM1_DUTY);
	}
	return sizeof(int);
}

static struct rtdm_device device = {
	.struct_version = RTDM_DEVICE_STRUCT_VER,

	.device_flags = RTDM_NAMED_DEVICE,
	.context_size = 0,
	.device_name = "xeno_encoder1",

	.open_nrt = encoder_open,

	.ops = {
		.close_rt = encoder_close,
		.close_nrt = encoder_close,
		.read_rt   = encoder_read_rt,
		.read_nrt  = encoder_read_rt,
		.write_rt = motor_write,
		.write_nrt = motor_write,
	},

	.device_class = RTDM_CLASS_EXPERIMENTAL,
	.device_sub_class = RTDM_SUBCLASS_GENERIC,
	.profile_version = 1,
	.driver_name = "xeno_encoder1",
	.driver_version = RTDM_DRIVER_VER(0, 0, 1),
	.peripheral_name = "rt-lkm-encoder1",
	.provider_name = "ISIR",
	.proc_name = device.device_name,
};

 
int __init encoder_init(void) {
/////////////////////////////////////////////////////////////////
//PWM2

	if(!gpio_is_valid(pin_pwm1)) 
		rtdm_printk(KERN_INFO "Invalid PWM1 gpio\n");
	
	if(gpio_request(pin_pwm1,"sysfs") != 0) 
		rtdm_printk(KERN_ERR "gpio_request_pwm1 error\n");

	gpio_direction_output(pin_pwm1, 0);
	gpio_export(pin_pwm1,false);

	// Création du thread PWM1
	if (rtdm_task_init(&PWM1, "PWM1", (rtdm_task_proc_t) PWM1_thread, NULL, 99, (nanosecs_rel_t) PWM_PERIOD) != 0)
		rtdm_printk(KERN_INFO "Error while creating PWM1\r\n");
//	if (rt_task_spawn(&PWM1, "PWM1", 0, 99, T_JOINABLE, &PWM1_thread, NULL )!= 0)
//		rtdm_printk(KERN_INFO "Error while creating PWM1\r\n");

	return rtdm_dev_register(&device);	
}

void __exit encoder_exit(void) {
	rtdm_task_destroy(&PWM1);
	rtdm_timer_destroy(&PWM1_timer);
//	rt_task_delete(&PWM1);
	gpio_unexport(pin_pwm1);
	gpio_free(pin_pwm1);

	rtdm_dev_unregister(&device,1000);
}

module_init(encoder_init);
module_exit(encoder_exit);
