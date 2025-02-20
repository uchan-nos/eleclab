all : flash

TARGET:=illumibed
ADDITIONAL_C_FILES:=switch_mgr.c

TARGET_MCU:=CH32V003

include ../env.mk
include $(CH32V003FUN)/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean

