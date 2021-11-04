#include <stdio.h>
#include "defs.h"
#include "axi-bram.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/time.h>

#define UIO_NAME "/dev/uio0"
#define LEN 1023

int fd;
u32 data[LEN];
volatile int cnt = 0;
struct timeval tic, toc;
double timeuse = 0;

void my_signal_fun()
{
    int on = 1;    // uio 使能命令
    int count = 4; // uio 中断驱动，必须4字节操作

    cnt++;
    gettimeofday(&tic, NULL);
    BramReadWords(0x50080000, LEN, 0x0, data, LEN);
    gettimeofday(&toc, NULL);
    timeuse += (toc.tv_sec - tic.tv_sec) + (double)(toc.tv_usec - tic.tv_usec) / 1000000.0;

    //        if (cnt >= 1000)
    //        {
    //            printf("%d\n",cnt);
    //	    fflush(stdout);
    //        }
    //	fprintf(stdout, "%d\n", cnt);
    write(fd, &on, count); // 使能uio中断
}

int main()
{
    int Oflags;

    signal(SIGIO, my_signal_fun);

    fd = open(UIO_NAME, O_RDWR);
    if (fd < 0)
    {
        printf("can't open file ,file name is %s!\n", UIO_NAME);
    }

    printf("open the %s success\n", UIO_NAME);

    fcntl(fd, F_SETOWN, getpid());
    Oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, Oflags | FASYNC);
    int on = 1;
    write(fd, &on, 4);

    while (1)
    {
        //                printf("cnt = %d\n",cnt);
        //                fflush(stdout);
        //		if (cnt == 1)
        //		{
        //			//tic = clock();
        //			gettimeofday(&tic, NULL);
        //		}

        if (cnt >= 1000)
        {
            //toc = clock();
            //gettimeofday(&toc, NULL);
            //printf("%d th data in %f\n", cnt, (double)s_time/CLOCKS_PER_SEC);
            //timeuse=(toc.tv_sec-tic.tv_sec)+(double)(toc.tv_usec-tic.tv_usec)/1000000.0;
            printf("%d th data in %f\n", cnt, timeuse);
            fflush(stdout);
            cnt = 0;
            timeuse = 0;
        }
        usleep(1);
    }

    return 0;
}
