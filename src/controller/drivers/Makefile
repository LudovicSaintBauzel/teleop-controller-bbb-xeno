obj-m += encoder.o

ccflags-y = $(shell xeno-config --skin posix --cflags)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
