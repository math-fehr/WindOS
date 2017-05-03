#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"

int main() {
  volatile unsigned int i;
  int test=42;
  printf("%#010x\n",(unsigned int)&test);
  scanf("%d",&test);
  for (i=0;i<25000000;i++) {}
  printf("Coucou! %d\n", test);
  return 0;
}
