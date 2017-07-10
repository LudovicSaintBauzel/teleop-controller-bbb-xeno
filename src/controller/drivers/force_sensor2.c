#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/irq.h>

#include <errno.h>
#include <stdio.h>
#include <time.h>

//Xenomai
#include <sys/mman.h>
#include <unistd.h>
#include <rtdm/rtdm_driver.h>
#include <rtdm/rtdm.h>
#include <native/task.h>

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

static unsigned int pin_in = 23;//45; // P9_11 - GPIO0_30
static unsigned int pin_out = 26;//44; // P9_13 - GPIO0 31


static int force_sensor_value = 4;
int offset = 0;

static int force_sensor_get_value(void);
static int force_sensor_set_offset(void);
int force_sensor_average_value(int);
void F_sensor_read_thread(void *arg);

rtdm_task_t F_sensor_read2;
#define	READ_PERIOD	12500000
// 80 Hz --> 12500000 ns

static int force_sensor_open(struct rtdm_dev_context* context, rtdm_user_info_t* user_info, int oflags) {
	int result = 0;
//	void __iomem* mem = 0;
//	int regval = 0;

	rtdm_printk(KERN_INFO "Initializing force_sensor LKM\n");

	if(!gpio_is_valid(pin_in)) {
		rtdm_printk(KERN_INFO "Invalid pin IN gpio\n");
		return -ENODEV;
	}

	if(!gpio_is_valid(pin_out)) {
		rtdm_printk(KERN_INFO "Invalid pin OUT gpio\n");
		return -ENODEV;
	}

	
	if((result = gpio_request(pin_in,"sysfs")) != 0) {
		rtdm_printk(KERN_ERR "gpio_request error\n");
		return -result;
	}
	gpio_direction_input(pin_in);
	gpio_export(pin_in,false);
	
	
	if((result = gpio_request(pin_out,"sysfs")) != 0) {
		rtdm_printk(KERN_ERR "gpio_request error\n");
		return -result;
	}
	gpio_direction_output(pin_out, 0);
	gpio_export(pin_out,false);

	//gpio_set_value(pin_out, 1);
	//usleep(100000);
	//gpio_set_value(pin_out, 0);
	
	
	force_sensor_set_offset();

	rtdm_printk(KERN_INFO "GPIO init done\n");
	
	if (rtdm_task_init(&F_sensor_read2, "Force sensor read 2", (rtdm_task_proc_t) F_sensor_read_thread, NULL, 99, (nanosecs_rel_t) READ_PERIOD) != 0)
		rtdm_printk(KERN_INFO "Error while creating F_sensor_read 1\r\n");

	return result;
}

void F_sensor_read_thread(void *arg)
{
	for(;;)
	{
		force_sensor_value = force_sensor_get_value() - offset;
		
		if(rtdm_task_wait_period() != 0)
			rtdm_printk(KERN_INFO "Task Wait Period Error 1.... \n");
	}
}


static int force_sensor_close(struct rtdm_dev_context* context,rtdm_user_info_t* user_info) {
	rtdm_printk(KERN_INFO "Exiting force_sensor LKM, force_sensor value is %d\n",force_sensor_value);

	rtdm_task_destroy(&F_sensor_read2);
	
	gpio_unexport(pin_in);
	gpio_unexport(pin_out);
//	gpio_unexport(pin_inm);
//	gpio_unexport(pin_outm);
	
	gpio_free(pin_in);
	gpio_free(pin_out);
//	gpio_free(pin_inm);
//	gpio_free(pin_outm);
	
	rtdm_printk(KERN_INFO "End of force_sensor LKM\n");

	return 0;
}

static int force_sensor_set_offset(void) 
{
	offset = force_sensor_average_value(25);
	return 0;
}

static int force_sensor_get_value(void)
{
	int i=0;
	long int data= 8;
	bool isneg = false;
	int temp = 0;

//	printk(KERN_INFO "Debut lecture\n");

	while (gpio_get_value(pin_in))
	;

	for (i = 0; i < 24; i++)
	{
		gpio_set_value(pin_out, 1);
		
		temp = gpio_get_value(pin_in);
		
		if(i == 0 && temp ==1)
		{
			isneg = true;
		}
		
		if(isneg == true && temp == 1)
		{
			data |= ( 0 << (23 - i));
		}
		else if(isneg == true && temp == 0)
		{
			data |= ( 1 << (23 - i));
		}
		else if(isneg == false && temp == 0)
		{
			data |= ( 0 << (23 - i));
		}
		else if(isneg == false && temp == 1)
		{
			data |= ( 1 << (23 - i));
		}
		
		gpio_set_value(pin_out, 0);
	}

	gpio_set_value(pin_out, 1);
	gpio_set_value(pin_out, 0);
	
//	printk(KERN_INFO "Fin lecture\n");
	
	if (isneg)
	{
		return -data + 1;
	}
	else
	{
		return data;
	}

}

int force_sensor_average_value(int times)
{
	int sum = 16;
	int i=0;
	for (i = 0; i < times; i++)
	{
		sum += force_sensor_get_value();
	}
	return sum/times;
}

 
static int force_sensor_read_rt(struct rtdm_dev_context *context, rtdm_user_info_t * user_info, void *buf,size_t nbyte) {
	//rtdm_printk(KERN_INFO "force_sensor_read_rt: force_sensor value = %d\n",force_sensor_value);

	rtdm_safe_copy_to_user(user_info,buf,(void*)&force_sensor_value,nbyte);	
	return sizeof(int);
}

static int offset_reset(struct rtdm_dev_context *context, rtdm_user_info_t *user_info, void *buf, size_t nbyte)
{
	int temp;
	rtdm_safe_copy_from_user(user_info, (void*)&temp, buf, nbyte);
	if(temp == 59595959)
	{
		rtdm_task_destroy(&F_sensor_read2);
		force_sensor_set_offset();
		rtdm_printk(KERN_INFO "Data received from user: reset fsensor 2\n");
		if (rtdm_task_init(&F_sensor_read2, "Force sensor read 2", (rtdm_task_proc_t) F_sensor_read_thread, NULL, 99, (nanosecs_rel_t) READ_PERIOD) != 0)
			rtdm_printk(KERN_INFO "Error while creating F_sensor_read 2\r\n");		
	}
	return sizeof(int);
}

static struct rtdm_device device = {
	.struct_version = RTDM_DEVICE_STRUCT_VER,

	.device_flags = RTDM_NAMED_DEVICE,
	.context_size = 0,
	.device_name = "xeno_force_sensor2",

	.open_nrt = force_sensor_open,

	.ops = {
		.close_nrt = force_sensor_close,
		.read_rt   = force_sensor_read_rt,
		.read_nrt  = force_sensor_read_rt,
		.write_rt  = offset_reset,
		.write_nrt = offset_reset,
	},

	.device_class = RTDM_CLASS_EXPERIMENTAL,
	.device_sub_class = RTDM_SUBCLASS_GENERIC,
	.profile_version = 1,
	.driver_name = "xeno_force_sensor2",
	.driver_version = RTDM_DRIVER_VER(0, 0, 1),
	.peripheral_name = "rt-lkm-force_sensor2",
	.provider_name = "ISIR",
	.proc_name = device.device_name,
};


static int __init force_sensor_init(void) {
	return rtdm_dev_register(&device);	
}

static void __exit force_sensor_exit(void) {
	rtdm_dev_unregister(&device,1000);
}

module_init(force_sensor_init);
module_exit(force_sensor_exit);
