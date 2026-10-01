# ==============================================================================
# MediSave Edge - Character Device Driver Makefile
# Builds the medisave_driver.ko Linux Kernel Module
# ==============================================================================

obj-m += medisave_driver.o

# Dynamic Linux Kernel Build Directory
KDIR ?= /lib/modules/$(shell uname -r 2>/dev/null)/build
PWD  := $(shell pwd)

.PHONY: all default clean help

default: all

all:
	@if [ -d "$(KDIR)" ]; then \
		echo "[Driver] Building medisave_driver.ko against kernel build tree: $(KDIR)..."; \
		$(MAKE) -C $(KDIR) M=$(PWD) modules; \
		echo "[Driver] Successfully compiled medisave_driver.ko"; \
	else \
		echo "===================================================================="; \
		echo " [Driver Notice] Linux Kernel build tree not detected at:"; \
		echo "   $(KDIR)"; \
		echo " To build this kernel module on a Linux system, ensure kernel headers"; \
		echo " are installed by running:"; \
		echo "   sudo apt install -y linux-headers-$$(uname -r) build-essential"; \
		echo "===================================================================="; \
	fi

clean:
	@if [ -d "$(KDIR)" ]; then \
		$(MAKE) -C $(KDIR) M=$(PWD) clean; \
	fi
	@rm -f *.o *.ko *.mod *.mod.c *.mod.o *.cmd .*.cmd modules.order Module.symvers
	@rm -rf .tmp_versions
	@echo "[Driver Clean] Kernel module build artifacts cleaned."

help:
	@echo "MediSave Driver Makefile Commands:"
	@echo "  make        - Build medisave_driver.ko kernel module"
	@echo "  make clean  - Clean kernel compilation artifacts"
