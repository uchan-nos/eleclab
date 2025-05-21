all: ui_test

ui_test: ui_test.c ../ui.c ../main.h
	gcc -g -rdynamic -o $@ ui_test.c ../ui.c

.PHONY: test
test: ui_test
	./ui_test
