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
	while(1) {
		char buf[1500];
		printf("$");
		scanf("%s",buf);
		if(strcmp(buf,"exit") == 0) {
			break;
		}
	}
	printf("Let's leave\n");
  	return 0;
}
