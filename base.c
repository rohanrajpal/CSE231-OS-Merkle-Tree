#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "filesys.h"

#define NUM_FILES 8
#define total_size_glob 128000
int main_loop (char *filename)
{
	int fd1, fd2;
	char buf[128];

	int size, total_size, ret;

	fd1 = s_open (filename, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
	if (fd1 == -1) {
		printf ("Unable to open file descriptor1\n");
		return 0;
	}

	memset (buf, 1, 128);

//    fill(buf,buf+128,66);
	total_size = total_size_glob;

	while (total_size) {
		size = (rand() % 127) + 1;
//		size = 64;
		if (size > total_size) {
			size = total_size;
		}
//		printf("base.c: Calling s_write\n");
        if(total_size == 64){
            printf("base.c:total size is 64\n");
        }

		ret = s_write (fd1, buf, size);
		if (ret != size) {
			printf ("Unable to write to file\n");
			return 0;
		}
		total_size -= size;
	}

	s_close (fd1);
//    printf("base.c : writing finished\n");
	fd2 = s_open (filename, O_RDONLY, 0);
	if (fd2 == -1) {
		printf ("Unable to open file descriptor2\n");
		return 0;
	}
	total_size = total_size_glob;
	while (total_size) {
//        printf("Base.c:2 total size is %d\n", total_size);

        size = (total_size > 128) ? 128 : total_size;
		ret = s_read (fd2, buf, size);

		if (ret != size) {
			printf ("Unable to read from file %d\n", ret);
			return 0;
		}
		total_size -= size;
//        printf("Base.c: total size is %d\n", total_size);

    }
//	printf("Base.c: read finished\n");
	s_close (fd2);
	return 0;
}

int main ()
{
	int i;
	char filename[32];

	system ("rm -rf foo*.txt");
	if (filesys_init() == 1) {
		printf ("Unable to init filesys\n");
		return 0;
	}
	for (i = 0; i < NUM_FILES; i++) {
		snprintf (filename, 32, "foo_%d.txt", i);
		main_loop (filename);
	}
	return 0;
}
