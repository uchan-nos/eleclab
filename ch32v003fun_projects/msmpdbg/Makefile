all : flash

TARGET:=msmpdbg
ADDITIONAL_C_FILES:=msmp_recorder.c
TARGET_MCU:=CH32V203K8T6
TARGET_MCU_PACKAGE:=CH32V203K8T6

include ../env.mk
include $(CH32V003FUN)/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean

show_macros: $(TARGET).c
	$(PREFIX)-gcc -E -dM $(CFLAGS) $< | grep '^#define FUNCONF' | sort -u

show_macros_all: $(TARGET).c
	$(PREFIX)-gcc -E -dM $(CFLAGS) $< | sort -u
