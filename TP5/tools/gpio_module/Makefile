# Nombre del modulo; debe coincidir con el nombre base del archivo .c.
obj-m := dual_gpio_input_module.o

# Arbol del kernel de Raspberry Pi preparado para cross-compiling.
KERNEL_DIR ?= $(CURDIR)/raspberrypi-linux

# Toolchain y arquitectura destino para Raspberry Pi 5.
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
CC := aarch64-linux-gnu-gcc-14

all:
	make -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CC=$(CC) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(CURDIR) modules

clean:
	make -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CC=$(CC) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(CURDIR) clean
