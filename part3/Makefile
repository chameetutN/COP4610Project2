obj-m += elevator.o
elevator-objs := src/elevator.o src/list.o

PWD := $(CURDIR)
KDIR ?= /lib/modules/$(shell uname -r)/build

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: default clean
