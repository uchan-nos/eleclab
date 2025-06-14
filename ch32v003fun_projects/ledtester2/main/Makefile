all : flash

TARGET:=main
TARGET_MCU:=CH32V003
ADDITIONAL_C_FILES:=periph.c pwctrl.c ui.c queue.c

include ../../env.mk
include $(CH32V003FUN)/ch32fun/ch32fun.mk

FLASH_COMMAND:=$(MINICHLINK)/minichlink -D -w $(TARGET).bin $(WRITE_SECTION) -b

flash : cv_flash
clean : cv_clean
