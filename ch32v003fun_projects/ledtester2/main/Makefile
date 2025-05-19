all : flash

TARGET:=main
TARGET_MCU:=CH32V003
ADDITIONAL_C_FILES:=periph.c pwctrl.c

include ../../env.mk
include $(CH32V003FUN)/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean
