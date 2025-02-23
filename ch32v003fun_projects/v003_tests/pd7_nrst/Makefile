all :
	# configure NRST as GPIO
	$(CH32V003FUN)/minichlink/minichlink -D
	# flash program
	make flash

TARGET:=pd7
TARGET_MCU:=CH32V003

include ../../env.mk
include $(CH32V003FUN)/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean
