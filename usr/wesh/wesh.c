#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"

int main() {
  	volatile unsigned int i;
  	int k=fork();

	if (k == 0) {
		printf("Je suis l'enfant!\n");
	} else {
		printf("Je suis le père de %d\n",k);
	}

  	while(1) {}
  	return 0;
}
