#include <linux/init.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <asm/io.h>
#include <asm/uaccess.h>	/* copy_{from,to}_user() */
#include <linux/sched.h>
#include <linux/cdev.h>  
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
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
#define AM33XX_CONTROL_BASE		0x44e10000
// spruh73m.pdf  Issue de L3 memory et L4 memory p179 p181 p182
#define GPIO0_REGISTER 0x44e07000
#define GPIO1_REGISTER 0x4804C000
#define GPIO2_REGISTER 0x481AC000
#define GPIO3_REGISTER 0x481AE000
#define GPIO3_START_ADDR 0x481AE000
#define GPIO_LEN (0x481AEfff-GPIO0_START_ADDR)

#define OFFSET_PIN1 0x848
#define GPIO_PIN1_R 18
#define GPIO_PIN1 50

#define OFFSET_PIN2 0x824
#define GPIO_PIN2_R 23
#define GPIO_PIN2 23

static unsigned int pin_in1 = GPIO_PIN1; // P9_14 - GPIO 50 - GPIO1-18
static unsigned int pin_in2 = GPIO_PIN2; 


static int contact_open(struct rtdm_dev_context* context, rtdm_user_info_t* user_info, int oflags) {
	return 0;
}


static int contact_close(struct rtdm_dev_context* context,rtdm_user_info_t* user_info) {	
	return 0;
}


static int contact_read_rt(struct rtdm_dev_context *context, rtdm_user_info_t * user_info, void *buf,size_t nbyte) {
	//rtdm_printk(KERN_INFO "contact_read_rt: contact value = %d\n",contact_value);
	int value1, value2;
	int ret = 0.;
	
	value1 = gpio_get_value(pin_in1);
	value2 = gpio_get_value(pin_in2);
	ret = value1*10 + value2;

	rtdm_safe_copy_to_user(user_info,buf,(void*)&ret, nbyte);	
	return sizeof(int);
}


static struct rtdm_device device = {
	.struct_version = RTDM_DEVICE_STRUCT_VER,

	.device_flags = RTDM_NAMED_DEVICE,
	.context_size = 0,
	.device_name = "xeno_contact1",

	.open_nrt = contact_open,

	.ops = {
		.close_nrt = contact_close,
		.read_rt   = contact_read_rt,
	},

	.device_class = RTDM_CLASS_EXPERIMENTAL,
	.device_sub_class = RTDM_SUBCLASS_GENERIC,
	.profile_version = 1,
	.driver_name = "xeno_contact1",
	.driver_version = RTDM_DRIVER_VER(0, 0, 1),
	.peripheral_name = "rt-lkm-contact1",
	.provider_name = "ISIR",
	.proc_name = device.device_name,
};


static int __init contact_init(void) {
	int ret=0;
	int regval ;                 
	void __iomem* cm_per_gpio;

	/********************* PINMUX spruh73m.pdf p. 179 - 1370 - 1426 ****************/
	/********************* INIT PIN 1 ****************/
	cm_per_gpio = ioremap(AM33XX_CONTROL_BASE+OFFSET_PIN1,4);
	if(!cm_per_gpio) {
	printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
	regval = ioread32(cm_per_gpio) ; //get GPIO MUX register value
	iowrite32( 0x7|(1<< 3)|(1<<5) ,cm_per_gpio);// p. 1426 : GPIO MUX to GPIO OUTPUT (Mode 7)
	//                                             Pullup (sel. and disabled)
	regval = ioread32(cm_per_gpio) ; //enabled? PWM GPIO MUX
	printk("GPX1CON register mux (1e) : 0x%08x\n", regval);

	}
	
	if(!gpio_is_valid(pin_in1)) {
		rtdm_printk(KERN_INFO "Invalid pin IN gpio\n");
	return -ENODEV;
	}

	if((ret = gpio_request(pin_in1,"sysfs")) != 0) {
		rtdm_printk(KERN_ERR "gpio_request error\n");
		return ret;
	}
	gpio_direction_input(pin_in1);
	gpio_export(pin_in1,false);
	printk("GPX1CON register mux (1e) : 0x%08x\n", regval);	

	/********************* INIT PIN 2 ****************/
	cm_per_gpio = ioremap(AM33XX_CONTROL_BASE+OFFSET_PIN2,4);
	if(!cm_per_gpio) {
	printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
	regval = ioread32(cm_per_gpio) ; //get GPIO MUX register value
	iowrite32( 0x7|(1<< 3)|(1<<5) ,cm_per_gpio);// p. 1426 : GPIO MUX to GPIO OUTPUT (Mode 7)
	//                                             Pullup (sel. and disabled)
	regval = ioread32(cm_per_gpio) ; //enabled? PWM GPIO MUX
	printk("GPX1CON register mux (2e) : 0x%08x\n", regval);

	}
	
	if(!gpio_is_valid(pin_in2)) {
		rtdm_printk(KERN_INFO "Invalid pin IN gpio\n");
	return -ENODEV;
	}

	if((ret = gpio_request(pin_in2,"sysfs")) != 0) {
		rtdm_printk(KERN_ERR "gpio_request error\n");
		return ret;
	}
	gpio_direction_input(pin_in2);
	gpio_export(pin_in2,false);
	printk("GPX1CON register mux (2e) : 0x%08x\n", regval);	
 
	return rtdm_dev_register(&device);	
}

static void __exit contact_exit(void) {
	rtdm_printk(KERN_INFO "Exiting contact security 1 LKM");
	gpio_unexport(pin_in1);	
	gpio_free(pin_in1);
	gpio_unexport(pin_in2);	
	gpio_free(pin_in2);		
	rtdm_dev_unregister(&device,1000);
}

module_init(contact_init);
module_exit(contact_exit);
