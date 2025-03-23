all : flash

TARGET:=adc_moving_avg
TARGET_MCU:=CH32V003

TARGET_MCU?=CH32V003
include ../../env.mk
include $(CH32V003FUN)/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean
