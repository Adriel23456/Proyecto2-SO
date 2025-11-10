obj-m += gpio_controller.o
obj-m += tft_driver.o

KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(shell pwd) modules
	gcc -o test_tft test_tft.c

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm -f test_tft