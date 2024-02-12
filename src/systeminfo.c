#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include "structures.h"
#include "helper.h"


extern PROCESS_LL * head;
extern struct final_print_struct *fps;
extern char MOUNT_POINT[256];
extern unsigned long long uptime;
extern long btime;
extern long clock_ticks_ps;

void getmeminfo(long long * memtotal, long long * memfree, long long * memavailable, long long * swaptotal, long long *swapfree)
{
    char filepath[264];
    snprintf(filepath,sizeof(filepath),"%s/meminfo",MOUNT_POINT);
    FILE * fp = fopen(filepath,"r");
    if(fp == NULL)
    {
        perror("getmeminfo: fopen");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    int c = 1;
    while ( fgets(buffer, sizeof(buffer), fp) != NULL )
    {
        if(c == 1)
            sscanf(buffer, "MemTotal: %lld", memtotal);
        else if (c == 2)
            sscanf(buffer, "MemFree: %lld", memfree);
        else if (c == 3)
        {
            sscanf(buffer, "MemAvailable: %lld", memavailable);
        }
        else if (c == 15)
        {
            sscanf(buffer, "SwapTotal: %lld", swaptotal);

        }
        else if (c == 16)
        {
           sscanf(buffer, "SwapFree: %lld", swapfree);
            break;
        }

        c++;

    }
    fclose(fp);
    if(c != 16)
    {
        fprintf(stderr,"unknown error reading meminfo");
        exit(EXIT_FAILURE);
    }
    

}




void get_mount_point()
{

	FILE * mtab = fopen("/etc/mtab","r");
	if(mtab == NULL)
	{
		if(errno == EACCES)	
		{
			snprintf(MOUNT_POINT,sizeof(MOUNT_POINT),"%s","/proc");
			return;
		}
		else 
		{
			perror("couldn't open mtab file");
			exit(EXIT_FAILURE);
		}
	}
	char line[1024];
	while(fgets(line,sizeof(line),mtab) != NULL)
	{
		if(sscanf(line,"proc %s",MOUNT_POINT))
		{
			fclose(mtab);
			return;
		}
	}

	fclose(mtab);
	fprintf(stderr,"couldn't find proc entry in mtab file");
	exit(EXIT_FAILURE);
}

void getuptime()
{
	char filepath[263];
	snprintf(filepath,sizeof(filepath),"%s/uptime",MOUNT_POINT);
    FILE * f=  fopen(filepath,"r");
     
    char b[256];
    if(fgets(b,sizeof(b),f) == NULL)
    {
		freeList(head);
		free(fps);
		endwin();
    	fprintf(stderr,"error getting uptime\n");
    	fclose(f);
    	exit(EXIT_FAILURE);
    }
    sscanf(b,"%llu", &uptime);
    fclose(f);
}

double readSystemCPUTime() 
{
	
	static int CPU_FIRST = 1;

	static struct cputotal
	{
		double seconds_prev;
		double seconds_cur;
	} CPU;
   
	char filepath[261];
	snprintf(filepath,sizeof(filepath),"%s/stat",MOUNT_POINT);
    FILE *stat_file = fopen(filepath, "r");
    if (stat_file == NULL) {
		freeList(head);
		free(fps);
		endwin();
        perror("error opening stat");
        exit(EXIT_FAILURE);
    }

    char line[256];
    if (fgets(line, sizeof(line), stat_file) == NULL) {
		freeList(head);
		free(fps);
		endwin();
        perror("error reading stat");
        fclose(stat_file);
        exit(EXIT_FAILURE);
    }

    fclose(stat_file);

  
    unsigned long user, nice, system, idle;
    sscanf(line, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);


    double s = (double)user / clock_ticks_ps + (double)system / clock_ticks_ps;

    if(CPU_FIRST)
    {
    	CPU_FIRST = 0;
    	CPU.seconds_prev = s;
    	CPU.seconds_cur = s;
    }
    else 
    {
    	CPU.seconds_prev = CPU.seconds_cur;
    	CPU.seconds_cur = s;
    }

    return (CPU.seconds_cur - CPU.seconds_prev) * 100;
    
}

void getcurrenttime(struct tm * s)
{
	time_t t;
	time(&t);
	localtime_r(&t, s);
}

/* vreme podizanja sistema, u unix vremenu */

void getbtime()
{
	char filepath[260];
	snprintf(filepath,sizeof(filepath),"%s/stat",MOUNT_POINT);
    FILE *sysstat = fopen(filepath,"r"); 
    if(sysstat == NULL)
    {
		freeList(head);
		free(fps);
		endwin();
    	perror("getbtime: fopen");
    	exit(EXIT_FAILURE);
    }
    char buf[1024];
    int line = 1;
    while(fgets(buf,sizeof(buf),sysstat) != NULL)
    {
		if(line == 5)
	    {
			sscanf(buf,"btime %ld", &btime);
			break;
		}
		line++;
    }

    fclose(sysstat);

    if (line != 5 )
    {
		freeList(head);
		free(fps);
		endwin();
    	fprintf(stderr, "error reading btime\n");
    	exit(EXIT_FAILURE);
    }
	
    


}

long long getTimeInMilliseconds()
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
}