#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main() {
	char buf[255];
	getcwd(buf, 255);
	printf("%s", buf);
}
 
