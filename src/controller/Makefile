###### CONFIGURATION ######

### List of applications to be build
APPLICATIONS = controller
#~ APPLICATIONS = test
#~ APPLICATIONS = read_force_sensor
### Note: to override the search path for the xeno-config script, use "make XENO=..."
 

### List of modules to be build
MODULES =  drivers/pwm 
##drivers/force_sensor1 drivers/force_sensor2 
##drivers/motor1 drivers/motor2
#~ MODULES = drivers/pwm_4MHz
### Note: to override the kernel source path, use "make KSRC=..."



###### USER SPACE BUILD (no change required normally) ######
ifeq ($(KERNELRELEASE),)
ifneq ($(APPLICATIONS),)

### Default Xenomai installation path
XENO ?= /usr/xenomai

XENOCONFIG=$(shell PATH=$(XENO):$(XENO)/bin:$(PATH) which xeno-config 2>/dev/null)

### Sanity check
ifeq ($(XENOCONFIG),)
all::
	@echo ">>> Invoke make like this: \"make XENO=/path/to/xeno-config\" <<<"
	@echo
endif


CC=$(shell $(XENOCONFIG) --cc)

CFLAGS=$(shell $(XENOCONFIG) --skin=native --cflags) $(MY_CFLAGS) -g3

LDFLAGS=$(MY_LDFLAGS)
LDLIBS=$(shell $(XENOCONFIG) --skin=native --ldflags) \
	$(shell $(XENOCONFIG) --skin=rtdm --ldflags)

# This includes the library path of given Xenomai into the binary to make live
# easier for beginners if Xenomai's libs are not in any default search path.
LDFLAGS+=-Xlinker -rpath -Xlinker $(shell $(XENOCONFIG) --libdir)
LDLIBS += -lm
#LDLIBS += -I/lib/modules/$(shell uname -r)/build/include/xenomai -I/lib/modules/$(shell uname -r)/build/include/xenomai/posix

all:: $(APPLICATIONS)

clean::
	$(RM) $(APPLICATIONS) *.o

endif
endif



###### KERNEL MODULE BUILD (no change required normally) ######
ifneq ($(MODULES),)

### Default to sources of currently running kernel
KSRC ?= /lib/modules/$(shell uname -r)/build

OBJS     := ${patsubst %, %.o, $(MODULES)}
CLEANMOD := ${patsubst %, .%*, $(MODULES)}
PWD      := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

### Kernel 2.6 or 3.0
PATCHLEVEL:=$(shell sed 's/PATCHLEVEL = \(.*\)/\1/;t;d' $(KSRC)/Makefile)
VERSION:=$(shell sed 's/VERSION = \(.*\)/\1/;t;d' $(KSRC)/Makefile)
ifneq ($(VERSION).$(PATCHLEVEL),2.4)

obj-m        := $(OBJS)
EXTRA_CFLAGS := -I$(KSRC)/include/xenomai -I$(KSRC)/include/xenomai/posix $(ADD_CFLAGS)

all::
	$(MAKE) -C $(KSRC) SUBDIRS=$(PWD) modules

### Kernel 2.4
else

ARCH    ?= $(shell uname -i)
INCLUDE := -I$(KSRC)/include/xenomai -I$(KSRC)/include/xenomai/compat -I$(KSRC)/include/xenomai/posix 
CFLAGS  += $(shell $(MAKE) -s -C $(KSRC) CC=$(CC) ARCH=$(ARCH) SUBDIRS=$(PWD) modules) $(INCLUDE)

all:: $(OBJS)

endif

## Target for capturing 2.4 module CFLAGS
modules:
	@echo "$(CFLAGS)"

clean::
#	find . -type f | xargs touch
	$(RM) $(CLEANMOD) *.o *.ko *.mod.c ./drivers/*.o ./drivers/*.ko ./drivers/*.mod.c ./drivers/.*.cmd Module*.symvers Module.markers modules.order
	$(RM) -R .tmp*


endif
