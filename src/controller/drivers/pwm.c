/*  Example  kernel module driver 
    by Ludovic Saint-Bauzel (saintbauzel@isir.upmc.fr)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/
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
#include <errno.h>
#include <stdio.h>

//Xenomai
#include <sys/mman.h>
#include <unistd.h>
#include <rtdm/rtdm_driver.h>

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <asm/io.h>

#include <asm/uaccess.h>	/* copy_{from,to}_user() */
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/cdev.h>  

#define AM33XX_CONTROL_BASE		0x44e10000
// spruh73m.pdf  Issue de L3 memory et L4 memory p179 p181 p182
#define GPIO0_REGISTER 0x44e07000
#define GPIO1_REGISTER 0x4804C000
#define GPIO2_REGISTER 0x481AC000
#define GPIO3_REGISTER 0x481AE000


// p. 183 
#define PWMSS0_REG 0x48300000
#define PWMSS1_REG 0x48302000
#define PWMSS2_REG 0x48304000

#define ECAP_OFF 0x100
#define EQEP_OFF 0x180
#define EPWM_OFF 0x200



// spruh73m.pdf Issue de register description GPIO p 4881 ...
#define GPIO_OE 0x134
#define GPIO_DATAIN 0x138
#define GPIO_DATAOUT 0x13C
#define GPIO_CTRL 0x130
#define GPIO_CLRDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194


// spruh73m.pdf Issue de p 1369
#define OFFSET_PWMSS_CTRL 0x664


#define OFFSET_PIN_8_34 0x8cc	//PWM1B -mode 2
#define OFFSET_PIN_8_36 0x8c8	//PWM1A -mode 2
#define OFFSET_PIN_9_14 0x848	//PWM1A -mode 6
#define OFFSET_PIN_9_22 0x950	//PWM0A -mode 3
#define OFFSET_PIN_9_31 0x990	//PWM0A -mode 1
#define OFFSET_PIN_8_45 0x8a0	//pwm2A -mode 3
#define OFFSET_PIN_8_46 0x8a4	//pwm2B -mode 3


#define EPWM_TBCTL 0x00
// Bits in TBCTL 
#define CTRMODE 0x0
#define PHSEN 0x2
#define PRDLD 0x3
#define SYNCOSEL 0x4
#define HSPCLKDIV 0x7
#define CLKDIV 0xA 
// Values in TBCTL
#define TB_UP 0x0
#define TB_UPDOWN 0x2 
#define TB_DISABLE 0x0 
#define TB_SHADOW 0x0
#define TB_SYNC_DISABLE 0x3
#define TB_DIV1 0x0
#define TB_DIV2 0x1
#define TB_DIV4 0x2

#define EPWM_CMPCTL 0x0E
// Bits in CMPCTL
#define SHDWAMODE 0x4
#define SHDWBMODE 0x6
#define LOADAMODE 0x0
#define LOADBMODE 0x2
// Values in CMPCTL
#define CC_SHADOW 0x0
#define CC_CTR_ZERO 0x0

#define EPWM_TBCNT 0x08
#define EPWM_TBPRD 0x0A
#define EPWM_TBPHS 0x06

#define EPWM_CMPA 0x12
#define EPWM_CMPB 0x14

#define EPWM_AQCTLA 0x16
#define EPWM_AQCTLB 0x18
// Bits in AQCTL
#define ZRO 0
#define PRD 2
#define CAU 4
#define CAD 6
#define CBU 8
#define CBD 10
// Values
#define AQ_CLEAR 0x1
#define AQ_SET 0x2



//#define ECAP_OFF 0x100
/* ECAP registers and bits definitions */
#define CAP1			0x08
#define CAP2			0x0C
#define CAP3			0x10
#define CAP4			0x14
#define ECCTL2			0x2A
#define ECCTL2_APWM_POL_LOW     (0x1 << 10)
#define ECCTL2_APWM_MODE        (0x1 << 9)
#define ECCTL2_SYNC_SEL_DISA	((0x1 << 7) |(0x1 << 6))
#define ECCTL2_TSCTR_FREERUN	(0x1 << 4)

#define INTC_MIR_CLEAR2 0xC8
#define INTC_MIR_SET2 0xCC
#define EPWM0INT 86 
#define EPWM1INT 87

int PERIOD = 10000;
int PERIOD_CLOCK = 26;
int BUSY_CLOCK = 0x0D;


MODULE_DESCRIPTION("Simple ioctl pwm control driver (char)");
MODULE_AUTHOR("Ludovic Saint-Bauzel, ISIR UPMC");
MODULE_LICENSE("GPL");

// CONTROLE MODULE

#define CONTROLE_MODULE_BASE_ADDR 0x44e10000
#define CONTROLE_MODULE_LEN 0x2000
//pwmss ? 

// CLOCK MANAGEMENT

#define CM_PER_BASE_ADDR 0x44e00000
#define CM_PER_LEN 0x4000

#define CM_PER_L4LS_CLKSTCTRL 0
#define CM_PER_L4LS_CLKCTRL 0x60

#define CM_PER_CLKCTRL_LEN 0x04
// POWER RESET MODULE PERIPHERAL REGISTERS

#define PRM_PER_BASE_ADDR 0x44e00c00 

#define PM_PER_PWRSTST 0x8
#define PM_PER_PWRSTCTRL 0xc

static void bbb_l4ls_config(void) {
  void __iomem* cm_per_l4ls_clkctrl_reg;
  void __iomem* cm_per_l4ls_clkstctrl_reg;
  int regval = 0;

  if(!request_mem_region(CM_PER_BASE_ADDR + CM_PER_L4LS_CLKCTRL,4,"cm_per_l4ls_clkctrl")) {
    printk(KERN_ERR "request_mem_region: cm_per_l4ls_clkctrl\n");
  }
  else {
    cm_per_l4ls_clkctrl_reg = ioremap(CM_PER_BASE_ADDR + CM_PER_L4LS_CLKCTRL, 4);
    if(!cm_per_l4ls_clkctrl_reg) {
      printk(KERN_ERR "cm_per_l4ls_clkctrl_reg ioremap: error\n");
    }
    else {
      regval = ioread32(cm_per_l4ls_clkctrl_reg);
      printk(KERN_INFO "cm_per_l4ls_clkctrl: %08x\n",regval);
      iowrite32(regval|0x2,cm_per_l4ls_clkctrl_reg); // Enable module
      while((ioread32(cm_per_l4ls_clkctrl_reg)&0x0300)!=0); // Fully functional
      iounmap(cm_per_l4ls_clkctrl_reg);
    }
    release_mem_region(CM_PER_BASE_ADDR + CM_PER_L4LS_CLKCTRL,4);
  }

  if(!request_mem_region(CM_PER_BASE_ADDR+CM_PER_L4LS_CLKSTCTRL,4,"cm_per_l4ls_clksctrl")) {
    printk(KERN_ERR "request_mem_region: cm_per_l4ls_clksctrl\n");
  }
  else {
    cm_per_l4ls_clkstctrl_reg = ioremap(CM_PER_BASE_ADDR+CM_PER_L4LS_CLKSTCTRL,4);
    if(!cm_per_l4ls_clkstctrl_reg) {
      printk(KERN_ERR "cm_per_l4ls_clkctrl_reg ioremap: error\n");
    }
    else {
      printk(KERN_INFO "cm_per_l4ls_clkstctrl: %08x\n",ioread32(cm_per_l4ls_clkstctrl_reg));
      iounmap(cm_per_l4ls_clkstctrl_reg);
    }
    release_mem_region(CM_PER_BASE_ADDR+CM_PER_L4LS_CLKSTCTRL,4);

  }
}

static void bbb_pwmss_config_clk(void) {

  void __iomem* cm_per_pwmss_reg;
  int regval = 0;
  int clkctrl_offset[] = {0xD4,0xCC,0xD8};
  int ii=0;

  for (ii=0;ii<3;ii++){
  if(!request_mem_region(CM_PER_BASE_ADDR + clkctrl_offset[ii],4,"cm_per_pwmssn_clkctrl")) {
    printk(KERN_ERR "request_mem_region: cm_per_pwmss%d_clkctrl\n",ii);
  }
  else {
    cm_per_pwmss_reg = ioremap(CM_PER_BASE_ADDR + clkctrl_offset[ii], CM_PER_CLKCTRL_LEN);
    if(!cm_per_pwmss_reg) {
      printk(KERN_ERR "cm_per_l4ls_clkctrl_reg ioremap: error\n");
    }
    else {
      regval = ioread32(cm_per_pwmss_reg);
      printk(KERN_INFO "cm_per_pwmssx_clkctrl: %08x\n",regval);
      iowrite32(regval|0x2,cm_per_pwmss_reg); // Enable module
      while((ioread32(cm_per_pwmss_reg)&0x0300)!=0); // Fully functional
      iounmap(cm_per_pwmss_reg);
    }
    release_mem_region(CM_PER_BASE_ADDR + clkctrl_offset[ii],4);
  }}
}

static int pwm_open(struct rtdm_dev_context* context, rtdm_user_info_t* user_info, int oflags) {
	printk(KERN_INFO "PWM device opened\n");
	return 0;
}


static int pwm_close(struct rtdm_dev_context* context,rtdm_user_info_t* user_info) {	
	printk(KERN_INFO "PWM device closed\n");
	return 0;
}


static long char_ioctl(struct rtdm_dev_context* context, rtdm_user_info_t* user_info, unsigned int cmd, unsigned long arg)
{
	void __iomem* cm_per_gpio;
	short sregval;
	unsigned short val;
	//~ printk( KERN_DEBUG "char5: ioctl() PWMSET(0x%04x)\n",(unsigned short)arg);

	val=(unsigned short)arg;
	if(val > PERIOD)
		val = PERIOD;
		
	switch (cmd) {
		case 0 :
			cm_per_gpio = ioremap(PWMSS1_REG + EPWM_OFF + EPWM_CMPA,4);
			if(!cm_per_gpio) {
				printk("cm_per_gpio_reg ioremap: error\n");
				break;
			}
			else {
				iowrite16(val,cm_per_gpio);
				sregval = ioread16(cm_per_gpio);
				//~ printk("DUTY 0 0x%04x\n",sregval);
			}
			break;
		
		case 1:
			cm_per_gpio = ioremap(PWMSS2_REG + EPWM_OFF + EPWM_CMPA,4); // p. 2325 : Get  Counter Compare  Register
			//           for output A
			if(!cm_per_gpio) {
				printk("cm_per_gpio_reg ioremap: error\n");
			}
			else {
				iowrite16(val,cm_per_gpio);
				sregval = ioread16(cm_per_gpio);
				//~ printk("DUTY 1 0x%04x\n",sregval);
			}
			break;


		default :
			printk(KERN_WARNING "kpwm: 0x%x unsupported ioctl command\n", cmd);
			return -EINVAL;

	}
	return 0;
}


static int  pinmuxPWM(void)
{
	int ret=0;
	int regval ;                 
	void __iomem* cm_per_gpio;

	/********************* PINMUX spruh73m.pdf p. 179 - 1370 - 1426 ****************/
	cm_per_gpio = ioremap(AM33XX_CONTROL_BASE+OFFSET_PIN_8_36,4);
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		regval = ioread32(cm_per_gpio) ; //get PWM GPIO MUX register value
		iowrite32(regval&(~(0xf | 0x3 << 4 )),cm_per_gpio); //reset PWM GPIO MUX
		regval = ioread32(cm_per_gpio) ; 
		iowrite32(regval|(0x2),cm_per_gpio);// p. 1426 : GPIO MUX to PWM (Mode 2)
		//                                             Pulldown (sel. and enabled)
		regval = ioread32(cm_per_gpio) ; //enabled? PWM GPIO MUX
		printk("GPX1CON register mux (1e) : 0x%08x\n", regval);

	}
	/********************* PINMUX spruh73m.pdf p. 179 - 1370 - 1426 ****************/
	cm_per_gpio = ioremap(AM33XX_CONTROL_BASE+OFFSET_PIN_8_45,4);
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		regval = ioread32(cm_per_gpio) ; //get PWM GPIO MUX register value
		iowrite32(regval&(~(0xf | 0x3 << 4 )),cm_per_gpio); //reset PWM GPIO MUX
		regval = ioread32(cm_per_gpio) ; 
		iowrite32(regval|(0x3),cm_per_gpio);// p. 1426 : GPIO MUX to PWM (Mode 3)
		//                                             Pulldown (sel. and enabled)
		regval = ioread32(cm_per_gpio) ; //enabled? PWM GPIO MUX
		printk("GPX1CON register mux (1e) : 0x%08x\n", regval);

	}
	/********************* PINMUX spruh73m.pdf p. 179 - 1370 - 1426 ****************/
	//~ cm_per_gpio = ioremap(AM33XX_CONTROL_BASE+OFFSET_PIN_9_31,4);
	//~ if(!cm_per_gpio) {
		//~ printk("cm_per_gpio_reg ioremap: error\n");
	//~ }
	//~ else {
		//~ regval = ioread32(cm_per_gpio) ; //get PWM GPIO MUX register value
		//~ iowrite32(regval&(~(0xf | 0x3 << 4 )),cm_per_gpio); //reset PWM GPIO MUX
		//~ regval = ioread32(cm_per_gpio) ; 
		//~ iowrite32(regval|(0x1),cm_per_gpio);// p. 1426 : GPIO MUX to PWM (Mode 1)
		//~ //                                             Pulldown (sel. and enabled)
		//~ regval = ioread32(cm_per_gpio) ; //enabled? PWM GPIO MUX
		//~ printk("GPX1CON register mux (1e) : 0x%08x\n", regval);
//~ 
	//~ }	
	return ret;
  
}

static int initPWM(void)
{
	int ret;
	int regval ;                 
	short sregval;
	void __iomem* cm_per_gpio;
	
	  /******* Enable L4LS and Clocks *******/
	bbb_l4ls_config();
	bbb_pwmss_config_clk();
	/********************* PWM enabling spruh73m.pdf p. 1369 - 1426 ****************/

	// P. 1369
	cm_per_gpio = ioremap(AM33XX_CONTROL_BASE+OFFSET_PWMSS_CTRL,4);
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		regval = ioread32(cm_per_gpio) ; 
		iowrite32(regval|0x6,cm_per_gpio); // p. 1407 : enable PWMSS1 (0x2) and PWMSS2 (0x4)

	}

	/********************* Enable PWMSS CLKCONFIG ****************/
   //~ cm_per_gpio = ioremap(PWMSS0_REG + EPWM_OFF + 0x80,2); // p. 2326 : see TBCTL
   //~ if(!cm_per_gpio) {
     //~ printk("cm_per_gpio_reg ioremap: error\n");
   //~ }
   //~ else {
     //~ iowrite16(0x0,cm_per_gpio); // Important de configurer le sens de comptage. Au démarrage il est freeze (0x83) 
     //~ sregval = ioread16(cm_per_gpio);
     //~ printk("TBCTL 0x%04x\n",sregval);
   //~ }
    
   cm_per_gpio = ioremap(PWMSS1_REG + EPWM_OFF + 0x80,2); // p. 2326 : see TBCTL
   if(!cm_per_gpio) {
     printk("cm_per_gpio_reg ioremap: error\n");
   }
   else {
     iowrite16(0x0,cm_per_gpio); // Important de configurer le sens de comptage. Au démarrage il est freeze (0x83) 
     sregval = ioread16(cm_per_gpio);
     printk("TBCTL 0x%04x\n",sregval);
   }

   cm_per_gpio = ioremap(PWMSS2_REG + EPWM_OFF + 0x80,2); // p. 2326 : see TBCTL
   if(!cm_per_gpio) {
     printk("cm_per_gpio_reg ioremap: error\n");
   }
   else {
     iowrite16(0x0,cm_per_gpio); // Important de configurer le sens de comptage. Au démarrage il est freeze (0x83) 
     sregval = ioread16(cm_per_gpio);
     printk("TBCTL 0x%04x\n",sregval);
   }
  


	/***************** PWM Configuration based on scenario p.2305 ************************/ 
	//~ cm_per_gpio = ioremap(PWMSS0_REG + EPWM_OFF + EPWM_AQCTLA,4); // p. 2325 : Get  Action Qualifier Register
	//~ //                                                                         for output A    
	//~ if(!cm_per_gpio) {
		//~ printk("cm_per_gpio_reg ioremap: error\n");
	//~ }
	//~ else {
		//~ sregval = ioread16(cm_per_gpio) ; //reset AQ Actions
		//~ iowrite16(0,cm_per_gpio);
		//~ sregval = ioread16(cm_per_gpio) ; //enable AQ Actions
		//~ iowrite16(sregval|(AQ_CLEAR << CAU | AQ_SET << PRD ),cm_per_gpio);
		//~ sregval = ioread16(cm_per_gpio) ; //read AQ Actions
		//~ printk("AQCTLA 0x%04x\n",sregval);
	//~ }
//~ 
	//~ cm_per_gpio = ioremap(PWMSS0_REG + EPWM_OFF + 0x1E,4); // p. 2325 : DeadBand Control Register
	//~ if(!cm_per_gpio) {
		//~ printk("cm_per_gpio_reg ioremap: error\n");
	//~ }
	//~ else {
		//~ sregval = ioread16(cm_per_gpio) ; //reset DeadBand
		//~ iowrite16(0,cm_per_gpio);
		//~ sregval = ioread16(cm_per_gpio);
		//~ printk("DBCTL 0x%04x\n",sregval);
	//~ }
//~ 
	//~ cm_per_gpio = ioremap(PWMSS0_REG + EPWM_OFF + EPWM_CMPA,4); // p. 2325 : Get  Counter Compare  Register
	//~ //           for output A
	//~ if(!cm_per_gpio) {
		//~ printk("cm_per_gpio_reg ioremap: error\n");
	//~ }
	//~ else {
		//~ iowrite16(BUSY_CLOCK,cm_per_gpio);
		//~ sregval = ioread16(cm_per_gpio);
		//~ printk("DUTY 0x%04x\n",sregval);
	//~ }
//~ 
	//~ cm_per_gpio = ioremap(PWMSS0_REG + EPWM_OFF + EPWM_TBPRD,4);  // p. 2325 : Get  Period  Register
	//~ //           for output A
	//~ if(!cm_per_gpio) {
		//~ printk("cm_per_gpio_reg ioremap: error\n");
	//~ }
	//~ else {
		//~ iowrite16(PERIOD_CLOCK,cm_per_gpio);
		//~ sregval = ioread16(cm_per_gpio);
		//~ printk("PERIOD 0x%04x\n",sregval);
	//~ }
	
	/*********************          PWM 1A      *************/
	cm_per_gpio = ioremap(PWMSS1_REG + EPWM_OFF + EPWM_AQCTLA,4); // p. 2325 : Get  Action Qualifier Register
	//                                                                         for output A    
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		sregval = ioread16(cm_per_gpio) ; //reset AQ Actions
		iowrite16(0,cm_per_gpio);
		sregval = ioread16(cm_per_gpio) ; //enable AQ Actions
		iowrite16(sregval|(AQ_CLEAR << CAU | AQ_SET << PRD ),cm_per_gpio);
		sregval = ioread16(cm_per_gpio) ; //read AQ Actions
		printk("AQCTLA 0x%04x\n",sregval);
	}

	cm_per_gpio = ioremap(PWMSS1_REG + EPWM_OFF + 0x1E,4); // p. 2325 : DeadBand Control Register
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		sregval = ioread16(cm_per_gpio) ; //reset DeadBand
		iowrite16(0,cm_per_gpio);
		sregval = ioread16(cm_per_gpio);
		printk("DBCTL 0x%04x\n",sregval);
	}

	cm_per_gpio = ioremap(PWMSS1_REG + EPWM_OFF + EPWM_CMPA,4); // p. 2325 : Get  Counter Compare  Register
	//           for output A
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		iowrite16(0x00,cm_per_gpio);
		sregval = ioread16(cm_per_gpio);
		printk("DUTY 0x%04x\n",sregval);
	}

	cm_per_gpio = ioremap(PWMSS1_REG + EPWM_OFF + EPWM_TBPRD,4);  // p. 2325 : Get  Period  Register
	//           for output A
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		iowrite16(PERIOD,cm_per_gpio);
		sregval = ioread16(cm_per_gpio);
		printk("PERIOD 0x%04x\n",sregval);
	}
	
	/***************** PWM 2A ************************/ 

	cm_per_gpio = ioremap(PWMSS2_REG + EPWM_OFF + EPWM_AQCTLA,4); // p. 2325 : Get  Action Qualifier Register
	//                                                                         for output A    
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		sregval = ioread16(cm_per_gpio) ; //reset AQ Actions
		iowrite16(0,cm_per_gpio);
		sregval = ioread16(cm_per_gpio) ; //enable AQ Actions
		iowrite16(sregval|(AQ_CLEAR << CAU | AQ_SET << PRD ),cm_per_gpio);
		sregval = ioread16(cm_per_gpio) ; //read AQ Actions
		printk("AQCTLA 0x%04x\n",sregval);
	}

	cm_per_gpio = ioremap(PWMSS2_REG + EPWM_OFF + 0x1E,4); // p. 2325 : DeadBand Control Register
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		sregval = ioread16(cm_per_gpio) ; //reset DeadBand
		iowrite16(0,cm_per_gpio);
		sregval = ioread16(cm_per_gpio);
		printk("DBCTL 0x%04x\n",sregval);
	}

	cm_per_gpio = ioremap(PWMSS2_REG + EPWM_OFF + EPWM_CMPA,4); // p. 2325 : Get  Counter Compare  Register
	//           for output A
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		iowrite16(0x00,cm_per_gpio);
		sregval = ioread16(cm_per_gpio);
		printk("DUTY 0x%04x\n",sregval);
	}

	cm_per_gpio = ioremap(PWMSS2_REG + EPWM_OFF + EPWM_TBPRD,4);  // p. 2325 : Get  Period  Register
	//           for output A
	if(!cm_per_gpio) {
		printk("cm_per_gpio_reg ioremap: error\n");
	}
	else {
		iowrite16(PERIOD,cm_per_gpio);
		sregval = ioread16(cm_per_gpio);
		printk("PERIOD 0x%04x\n",sregval);
	}
	return 0;
}

static struct rtdm_device device = {
	.struct_version = RTDM_DEVICE_STRUCT_VER,

	.device_flags = RTDM_NAMED_DEVICE,
	.context_size = 0,
	.device_name = "xeno_pwm",

	.open_nrt = pwm_open,

	.ops = {
		.close_rt = pwm_close,
		.close_nrt = pwm_close,
		.ioctl_rt = char_ioctl,
		.ioctl_nrt = char_ioctl,
	},

	.device_class = RTDM_CLASS_EXPERIMENTAL,
	.device_sub_class = RTDM_SUBCLASS_GENERIC,
	.profile_version = 1,
	.driver_name = "xeno_pwm",
	.driver_version = RTDM_DRIVER_VER(0, 0, 1),
	.peripheral_name = "rt-lkm-pwm",
	.provider_name = "ISIR",
	.proc_name = device.device_name,
};

// this gets called on module init
int __init kernmodex_init(void)
{
	int ret;
	/* int regval ;                  */
	/* short sregval; */
	/* void __iomem* cm_per_gpio; */

	printk(KERN_INFO "Loading example driver by Ludo...\n");
	ret = pinmuxPWM();
	if (ret < 0)
	{
		printk("problem in pinmux of PWM");
		return ret;
	}

	ret= initPWM();
	if (ret < 0)
	{
		printk("problem in pinmux of PWM");
		return ret;
	}
	return rtdm_dev_register(&device);

}


// this gets called when module is getting unloaded
static void __exit kernmodex_exit(void)
{
	rtdm_dev_unregister(&device,1000);
	printk(KERN_INFO "Example driver by Ludo removed.\n");
}

// setting which function to call on module init and exit
module_init(kernmodex_init);
module_exit(kernmodex_exit);


MODULE_VERSION("0.2");
