#include <stdio.h>

#include "mcc_generated_files/mcc.h"

void main(void) {
  SYSTEM_Initialize();
  printf("hello, reflow toaster!\n");

  char s[32];
  for (;;) {
    printf(">");
    scanf("%s", s);
    printf("input: '%s'\n", s);
  }
}