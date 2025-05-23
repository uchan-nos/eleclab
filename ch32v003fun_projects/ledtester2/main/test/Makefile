all: ui_test

ui_test: ui_test.c ../ui.c ../main.h
	$(CC) -g -o $@ -DEXPORT_STATIC_FUNC ui_test.c ../ui.c

.PHONY: test
test: ui_test
	./ui_test
