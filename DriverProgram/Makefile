# Kernel modules
obj-m += gpio_controller.o
obj-m += tft_driver.o

KDIR = /lib/modules/$(shell uname -r)/build

# User-space library and tools
LIBRARY = libtft.a
LIB_OBJ = libtft.o
TEST_PROG = test_tft
HISTOGRAM_GEN = generate_histogram

CC = gcc
AR = ar
CFLAGS = -Wall -O2
LDFLAGS = -lm

all: modules library tools

modules:
	make -C $(KDIR) M=$(shell pwd) modules

library: $(LIBRARY)

$(LIBRARY): $(LIB_OBJ)
	$(AR) rcs $@ $^
	@echo "Library $(LIBRARY) created successfully"

$(LIB_OBJ): libtft.c libtft.h
	$(CC) $(CFLAGS) -c libtft.c -o $(LIB_OBJ)

tools: $(TEST_PROG) $(HISTOGRAM_GEN)

$(TEST_PROG): test_tft.c $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ test_tft.c -L. -ltft $(LDFLAGS)

$(HISTOGRAM_GEN): generate_histogram.c
	$(CC) $(CFLAGS) -o $@ generate_histogram.c $(LDFLAGS)

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm -f $(LIBRARY) $(LIB_OBJ) $(TEST_PROG) $(HISTOGRAM_GEN)
	rm -f *.o histogram.cvc *.dat

install_library:
	@echo "Installing library to /usr/local/lib"
	sudo cp $(LIBRARY) /usr/local/lib/
	sudo cp libtft.h /usr/local/include/
	@echo "Library installed. Use -ltft to link"

.PHONY: all modules library tools clean install_library