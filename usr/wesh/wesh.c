#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"
#include "errno.h"
#include "time.h"

extern int argc;
extern char** argv;
extern char** envp;

int main() {
	int compteur = 0;
	int r1 = _fork();
	int r2 = _fork();
	int r3 = _fork();
	int r4 = _fork();
	while(1) {
		printf("%d %d %d %d\n",r1,r2,r3,r4);
		for(int i=0;i<1500000;i++) {}
	}
	printf("Let's leave\n");
  	return 0;
}
