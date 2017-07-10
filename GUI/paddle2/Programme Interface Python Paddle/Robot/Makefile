####################################################################
# MAKEFILE TEMPLATE FOR KERNEL MODULES AND USER SPACE APPLICATIONS #
# V0.3 (C) 2006 www.captain.at #####################################
####################################################################

### Names of the kernel module(s) and user space application(s) ####
###   separated by spaces ##########################################
### (omit names if no module(s) or application(s) is(are) built) ###
MODULENAME        ?= 
KERNELMODULES     ?= 
USERAPPS          ?= controller_s
### SKIN = xeno, posix etc.
SKIN              ?= alchemy
UNAME             := $(shell uname -r)
KERNELSOURCEDIR   ?= /lib/modules/$(UNAME)/build

### Xenomai directory, xeno-config and library directory ###########
XENO_DIR          ?= /usr/xenomai
XENO_CONFIG       ?= $(XENO_DIR)/bin/xeno-config
XENO_LIB_DIR      ?= $(shell $(XENO_CONFIG) --library-dir)

### User space application compile options #########################
USERAPP_LIBS      ?=  -lalchemy 
USERAPP_LDFLAGS   ?= $(shell $(XENO_CONFIG) --$(SKIN) --ldflags) 
USERAPP_CFLAGS    ?= $(shell $(XENO_CONFIG) --$(SKIN) --cflags) -g

### General configuration stuff ####################################
CC                =  $(shell $(XENO_CONFIG) --cc)


############### KERNEL MODULE END ##################################

################ USER APPLICATIONS START ###########################
ifneq ($(USERAPPS),)

USERSP := ${patsubst %, %.c, ./sources/$(USERAPPS)}
./bin/$(USERAPPS): $(USERSP) ./bin/s526.o  
	$(CC) -g -o $@ $< ./bin/s526.o $(USERAPP_CFLAGS) $(USERAPP_LDFLAGS) $(USERAPP_LIBS) -lm


./bin/s526.o : ./sources/s526.c
	$(CC) -g -o $@ -c $< $(USERAPP_CFLAGS)

./bin/divers.o : ./sources/divers.c
	$(CC) -g -o $@ -c $< $(USERAPP_CFLAGS)


endif

clean::
	$(RM) ./bin/$(USERAPPS) ./bin/*.o
################# USER APPLICATIONS END ############################

.PHONY: clean
