#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define BUF_LEN 80

int main(){
	int fd = open("/dev/ruslandev",O_RDONLY);
	char buf[BUF_LEN];
	if( fd < 0 )
		printf("Couldnt open file\n");
	if( read(fd,buf,BUF_LEN) != BUF_LEN )
		printf("Couldnt read file\n");
	printf("%s\n", buf);
	return 0;
}
