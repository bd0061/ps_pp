#include<ncurses.h>
#include<pthread.h>
#include<sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include "structures.h"
#include "infocollect.h"
#include "helper.h"
#include "parseopts.h"
#include "regex_module.h"
#include <errno.h>
#include <sys/stat.h>
#include "configreader.h"
#define tabbord (i == buflength - 1 ? "|" : "")

extern int COLOR_BG[3];
extern int COLOR_TEXT[3];
extern int COLOR_PRCNT_BAR[3];
extern int COLOR_SELECTPROC[3];
extern int COLOR_INFOTEXT[3];
extern int COLOR_INFO_SUC[3];
extern int COLOR_INFO_ERR[3];


extern char QUIT_KEY;
extern char KILL_KEY;
extern char IO_KEY;
extern char ID_KEY;
extern char CLASSIC_KEY;
extern char MEMSORT_KEY;
extern char CPUSORT_KEY;
extern char NORMALSORT_KEY;
extern char CUSTOM_KEY;

PROCESS_LL * head;
PROCESS_LL * start;

int memSort;
int normalSort;
int cpuSort;


int countDown;
int NOCOUNTDOWN;
char INFOMSG[64];
int saved;

int jFormat;
int ioFormat;
int clscFormat;
int customFormat;
int SUCCESS;


int selectedLine;
int x,y,cursx,cursy;
int start_pspp;

void* cursorUpdate(void * arg)
{
	while(1)
	{
		getmaxyx(stdscr,y,x);
		move(selectedLine - start_pspp,0);
		getyx(stdscr,cursy,cursx);
		napms(10);
	}
}


long long getTimeInMilliseconds()
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
}


void curse_init()
{
	readvals();

	srand(time(NULL));
	initscr();
	raw();
	noecho();
	curs_set(0);
	keypad(stdscr,TRUE);
	timeout(1000);
    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        exit(EXIT_FAILURE);
    }
	start_color();
	
	init_color(COLOR_BLACK,COLOR_BG[0],COLOR_BG[1],COLOR_BG[2]); //COLOR_BG
	init_color(COLOR_BLUE,COLOR_TEXT[0],COLOR_TEXT[1],COLOR_TEXT[2]); //COLOR_TEXT
	init_color(COLOR_RED,COLOR_PRCNT_BAR[0],COLOR_PRCNT_BAR[1],COLOR_PRCNT_BAR[2]); //COLOR_PRCNT_BAR
	init_color(COLOR_CYAN,COLOR_SELECTPROC[0],COLOR_SELECTPROC[1],COLOR_SELECTPROC[2]); //COLOR_SELECTPROC
	init_color(COLOR_MAGENTA,COLOR_INFOTEXT[0],COLOR_INFOTEXT[1],COLOR_INFOTEXT[2]); //COLOR_INFOTEXT
	init_color(COLOR_GREEN,COLOR_INFO_SUC[0],COLOR_INFO_SUC[1],COLOR_INFO_SUC[2]); //COLOR_INFO_SUC
	init_color(COLOR_YELLOW,COLOR_INFO_ERR[0],COLOR_INFO_ERR[1],COLOR_INFO_ERR[2]); // COLOR_INFO_ERR*/


	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_RED, COLOR_BLACK);
	init_pair(3,COLOR_BLACK,COLOR_CYAN);
	init_pair(4,COLOR_WHITE,COLOR_MAGENTA);
	init_pair(5,COLOR_WHITE,COLOR_GREEN);
	init_pair(6,COLOR_WHITE,COLOR_YELLOW);
	attron(COLOR_PAIR(1));
}


struct final_print_struct
{
	char mesg[2048];
	int pid;
};
struct final_print_struct *fps;
int fps_size;

void add_final(char * mesg, int pid)
{
    struct final_print_struct new_struct;
    snprintf(new_struct.mesg, sizeof(new_struct.mesg), "%s", mesg);
    new_struct.pid = pid;
    fps_size++;
  	
  	fps = realloc(fps, fps_size * sizeof(struct final_print_struct));

    if (fps == NULL) {
		freeList(head);
		free(fps);
		regfree(&regex);
		endwin();
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    fps[fps_size - 1] = new_struct;
}

void clear_and_reset_array(struct final_print_struct **array, int *array_size) {
    free(*array);
    *array_size = 0;
    *array = NULL;
}













int GLOBAL_CURSE_OFFSET;
double cpu_percent;



int printno;
int formatvals[23];
long clock_ticks_ps;
long pgsz;
long long memtotal, memfree, memavailable,swaptotal,swapfree;
unsigned long long cputicks;
int hahaxd;
long btime;
unsigned long uptime;
unsigned long long allprocs;
unsigned long long countprocs;

int CPU_FIRST = 1;



int check_vm() //VMWare
{
    unsigned int eax, ebx, ecx, edx;

    __asm__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    
    return ((ecx & (1 << 31)) != 0);

}




struct cputotal
{
	double seconds_prev;
	double seconds_cur;
} CPU;

double readSystemCPUTime() {

    FILE *stat_file = fopen("/proc/stat", "r");
    if (stat_file == NULL) {
		freeList(head);
		free(fps);
		regfree(&regex);
		endwin();
        perror("Error opening /proc/stat");
        exit(EXIT_FAILURE);
    }

    char line[256];
    if (fgets(line, sizeof(line), stat_file) == NULL) {
		freeList(head);
		free(fps);
		regfree(&regex);
		endwin();
        perror("Error reading /proc/stat");
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

void handle_io(char * dest, size_t destsize, long long target, int blocked, int pers)
{
	double vd;
	char * s = ""; 
	if(target > 1000000LL)
	{
		vd = (double)target / 1000000LL;
		s = "M";
	}
	else if (target > 1000LL)
	{
		vd = (double)target / 1000LL;
		s = "K";
	}
	else 
	{
		vd = (double)target;
	}

	if(blocked)
	{
		snprintf(dest,sizeof(dest),"%s", "N/A");

	}
	else 
	{
		snprintf(dest, destsize, "%.1lf %sB%s", vd,s, pers ? "/s" : "");
	}

}


void getuptime()
{
    FILE * f=  fopen("/proc/uptime","r");
     
    char b[256];
    if(fgets(b,sizeof(b),f) == NULL)
    {
		freeList(head);
		free(fps);
		regfree(&regex);
		endwin();
    	fprintf(stderr,"error getting uptime\n");
    	fclose(f);
    	exit(EXIT_FAILURE);
    }
    sscanf(b,"%llu", &uptime);
    fclose(f);
}



char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


void getcurrenttime(struct tm * s)
{
	time_t t;
	time(&t);
	localtime_r(&t, s);


}

/* vreme podizanja sistema, u unix vremenu */

void getbtime()
{
    FILE *sysstat = fopen("/proc/stat","r"); 
    if(sysstat == NULL)
    {
		freeList(head);
		free(fps);
		regfree(&regex);
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

    if (line != 5 )
    {
		freeList(head);
		free(fps);
		regfree(&regex);
		endwin();
    	fprintf(stderr, "error reading btime\n");
    	fclose(sysstat);
    	exit(EXIT_FAILURE);
    }
	
    fclose(sysstat);


}

/*ispravno istampaj znakove dekoracije u konacnom outputu */
void printequals(char ** buffer, int buflength)
{

	int toprint = 0;
	for(int i = 0; i < buflength; i++)
	{
			if 		(strcmp(buffer[i],"PID") == 0)		toprint += formatvals[0] +1;	
			else if (strcmp(buffer[i],"NAME") == 0) 	toprint += formatvals[1] + 1;		
			else if (strcmp(buffer[i],"STATE") == 0) 	toprint += formatvals[2]+ 1;	
			else if (strcmp(buffer[i],"PPID") == 0) 	toprint += formatvals[3] + 1;	
			else if (strcmp(buffer[i],"TTY") == 0) 		toprint += formatvals[4] + 1;	
			else if (strcmp(buffer[i],"UTIME") == 0) 	toprint += formatvals[5] + 1;	
			else if (strcmp(buffer[i],"STIME") == 0)  	toprint += formatvals[6] + 1;	
			else if (strcmp(buffer[i],"PRIO") == 0) 	toprint += formatvals[7] + 1;	
			else if (strcmp(buffer[i],"NICE") == 0)  	toprint += formatvals[8] + 1;	
			else if (strcmp(buffer[i],"THREADNO") == 0) toprint += formatvals[9] + 1;	
			else if (strcmp(buffer[i],"VIRT") == 0) 	toprint += formatvals[10] + 1;	
			else if (strcmp(buffer[i],"RES") == 0) 		toprint += formatvals[11] + 1;	
			else if (strcmp(buffer[i],"OWNER") == 0) 	toprint += formatvals[12] + 1;	
			else if (strcmp(buffer[i],"MEM%") == 0) 	toprint += formatvals[13] + 1;	
			else if (strcmp(buffer[i],"CPU%") == 0) 	toprint += formatvals[14] + 1;	
			else if (strcmp(buffer[i],"START") == 0) 	toprint += formatvals[15] + 1;	
			else if (strcmp(buffer[i],"SID") == 0) 		toprint += formatvals[16] + 1;	
			else if (strcmp(buffer[i],"PGRP") == 0) 	toprint += formatvals[17] + 1;	
			else if (strcmp(buffer[i],"C_WRITE") == 0) 	toprint += formatvals[18] + 1;	
			else if (strcmp(buffer[i],"IO_READ") == 0) 	toprint += formatvals[19] + 1;	
			else if (strcmp(buffer[i],"IO_WRITE") == 0) toprint += formatvals[20] + 1;	
			else if (strcmp(buffer[i],"IO_READ/s") == 0)toprint += formatvals[21] + 1;	
			else if (strcmp(buffer[i],"IO_WRITE/s") == 0) toprint += formatvals[22] + 1;
	}
	int j;
	for(j = 0; j < toprint; j++)
	{
		mvprintw(GLOBAL_CURSE_OFFSET,j,"=");
	}
	mvprintw(GLOBAL_CURSE_OFFSET++,j,"\n");


}
//zaboravio sam cemu je svrha ovoga
void getprintno(char ** buffer, int buflength)
{
	for(int i = 0; i < buflength; i++)
	{
			if 		(strcmp(buffer[i],"PID") == 0)		printno += formatvals[0] +1;	
			else if (strcmp(buffer[i],"NAME") == 0) 	printno += formatvals[1] + 1;		
			else if (strcmp(buffer[i],"STATE") == 0) 	printno += formatvals[2]+ 1;	
			else if (strcmp(buffer[i],"PPID") == 0) 	printno += formatvals[3] + 1;	
			else if (strcmp(buffer[i],"TTY") == 0) 		printno += formatvals[4] + 1;	
			else if (strcmp(buffer[i],"UTIME") == 0) 	printno += formatvals[5] + 1;	
			else if (strcmp(buffer[i],"STIME") == 0)  	printno += formatvals[6] + 1;	
			else if (strcmp(buffer[i],"PRIO") == 0) 	printno += formatvals[7] + 1;	
			else if (strcmp(buffer[i],"NICE") == 0)  	printno += formatvals[8] + 1;	
			else if (strcmp(buffer[i],"THREADNO") == 0) printno += formatvals[9] + 1;	
			else if (strcmp(buffer[i],"VIRT") == 0) 	printno += formatvals[10] + 1;	
			else if (strcmp(buffer[i],"RES") == 0) 		printno += formatvals[11] + 1;	
			else if (strcmp(buffer[i],"OWNER") == 0) 	printno += formatvals[12] + 1;	
			else if (strcmp(buffer[i],"MEM%") == 0) 	printno += formatvals[13] + 1;	
			else if (strcmp(buffer[i],"CPU%") == 0) 	printno += formatvals[14] + 1;	
			else if (strcmp(buffer[i],"START") == 0) 	printno += formatvals[15] + 1;	
			else if (strcmp(buffer[i],"SID") == 0) 		printno += formatvals[16] + 1;	
			else if (strcmp(buffer[i],"PGRP") == 0) 	printno += formatvals[17] + 1;	
			else if (strcmp(buffer[i],"C_WRITE") == 0) 	printno += formatvals[18] + 1;	
			else if (strcmp(buffer[i],"IO_READ") == 0) 	printno += formatvals[19] + 1;	
			else if (strcmp(buffer[i],"IO_WRITE") == 0) printno += formatvals[20] + 1;	
			else if (strcmp(buffer[i],"IO_READ/s") == 0)printno += formatvals[21] + 1;
			else if (strcmp(buffer[i],"IO_WRITE/s") == 0) printno += formatvals[22] + 1;	
	}
}

/*jednostavan algoritam za konverziju vremena u sekundama u dane, sate, minute, i preostale sekunde. 
(koristi se kod racunanje uptime-a)	*/
void convertseconds(unsigned long long seconds, int *days, int *hours, int *minutes, int *remaining_seconds) {
    *days = seconds / (24 * 3600);
    *remaining_seconds = seconds % (24 * 3600);
    *hours = *remaining_seconds / 3600;
    *remaining_seconds = *remaining_seconds % 3600;

    *minutes = *remaining_seconds / 60;
    *remaining_seconds = *remaining_seconds % 60;
}

/* odstampaj podatke o zauzetosti operativne memorije, iskoriscenost procesora,
*  iskoriscenost swap prostora, uptime sistema, i totalan broj procesa */
void print_stats()
{
 	int days, hours, minutes, seconds;
	convertseconds(uptime, &days, &hours, &minutes, &seconds);
	mvprintw(GLOBAL_CURSE_OFFSET++,0,"Uptime: %d day%s, %d hour%s, %d minute%s, %d second%s\n",days, days != 1 ? "s" : "", hours, hours != 1 ? "s" : "", 
		minutes, minutes != 1 ? "s" : "",seconds, seconds != 1 ? "s" : "");
	mvprintw(GLOBAL_CURSE_OFFSET++,0,"Processes on the system: %llu\n",allprocs);
	mvprintw(GLOBAL_CURSE_OFFSET++,0,"Of which displayed: %llu\n\n",countprocs);
	int barsize = 70;
	#ifdef __x86_64__
	if(check_vm())
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"(Note: this system may be a virtual machine.)\n");
	#else 
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"\n");
	#endif
	

	if(cpu_percent >= 100.0) cpu_percent = 100.0; //kompenzacija za moguce kasnjenje kupljenja informacija

	int onebar = 0;
	double scaled_cpu_percent = (cpu_percent * barsize) / 100;
	
	char hatethis1[1024];
	int len1 = snprintf(hatethis1,sizeof(hatethis1),"CPU: %4.1lf%%         ",cpu_percent);
	
	mvprintw(GLOBAL_CURSE_OFFSET,0,"%s",hatethis1);
	mvprintw(GLOBAL_CURSE_OFFSET,len1,"[");
	int i;
	for(i = 0; i < barsize; i++)
	{
		if(!onebar && scaled_cpu_percent > 0 && (int)scaled_cpu_percent == 0)
		{
			onebar = 1;
			attron(COLOR_PAIR(2));
			mvprintw(GLOBAL_CURSE_OFFSET,len1+i+1,"|");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(1));
			continue;
		}
		if(!onebar && i < (int)scaled_cpu_percent)
		{
			attron(COLOR_PAIR(2));
			mvprintw(GLOBAL_CURSE_OFFSET,len1+i+1,"|");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(1));
		}
		else
		{
			mvprintw(GLOBAL_CURSE_OFFSET,len1+i+1," ");
		}
	}
	mvprintw(GLOBAL_CURSE_OFFSET++,len1+i+1,"]\n");


	onebar = 0;
	double percent_memory = (double)(memtotal-memavailable)/memtotal * 100;
	//real prcnt : 100 = new_prcnt : barsize
   
	double scaled_percent_memory = (percent_memory * barsize)/100;
	
	char hatethis2[1024];
	int len2 = snprintf(hatethis2,sizeof(hatethis2),"Mem %.2lfGB/%.2lfGB  ", (double)(memtotal-memavailable)/1000000,(double)memtotal/1000000);

	mvprintw(GLOBAL_CURSE_OFFSET,0,"%s",hatethis2);

	mvprintw(GLOBAL_CURSE_OFFSET,len2,"[");
	i = 0;
	for(i = 0; i < barsize; i++)
	{
		if(!onebar && scaled_percent_memory > 0 && (int)scaled_percent_memory == 0)
		{
			onebar = 1;
			attron(COLOR_PAIR(2));
			mvprintw(GLOBAL_CURSE_OFFSET,len2+i+1,"|");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(1));
			continue;
		}
	    if(!onebar && i < (int)scaled_percent_memory)
	    {
	    	attron(COLOR_PAIR(2));
	        mvprintw(GLOBAL_CURSE_OFFSET,len2+i+1,"|");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(1));
	    }
	    else 
	    {
	        mvprintw(GLOBAL_CURSE_OFFSET,len2+i+1," ");
	    }
	}
	mvprintw(GLOBAL_CURSE_OFFSET++,len2+i+1,"]\n");




	double percent_swap = (double)(swaptotal-swapfree)/swaptotal * 100;
	//real prcnt : 100 = new_prcnt : barsize

	double scaled_percent_swap = (percent_swap * barsize) / 100;

	char hatethis3[1024];

	int len3 = snprintf(hatethis3,sizeof(hatethis3),
		"Swap %.2lf%s/%.2lfGB ", 
		swaptotal - swapfree < swaptotal/2 ? (double)(swaptotal-swapfree)/1000 : (double)(swaptotal-swapfree)/1000000,
		swaptotal - swapfree < swaptotal/2 ? "KB": "GB",
		(double)swaptotal/1000000);
	


	mvprintw(GLOBAL_CURSE_OFFSET,0,"%s",hatethis3);

	mvprintw(GLOBAL_CURSE_OFFSET,len3,"[");
	onebar = 0;
	
	i=0;
	for(i = 0; i < barsize; i++)
	{
		//hak da se bar jedna crtica ispise za vrlo male vrednosti
		if(!onebar && scaled_percent_swap > 0 && (int)scaled_percent_swap == 0)
		{
			onebar = 1;
			attron(COLOR_PAIR(2));
			mvprintw(GLOBAL_CURSE_OFFSET,len3+i+1,"|");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(1));
			continue;
		}
	    if(!onebar && i < (int)scaled_percent_swap)
	    {
	    	attron(COLOR_PAIR(2));
	        mvprintw(GLOBAL_CURSE_OFFSET,len3+i+1,"|");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(1));
	    }
	    else 
	    {
	        mvprintw(GLOBAL_CURSE_OFFSET,len3+i+1," ");
	    }
	}

	
	
	mvprintw(GLOBAL_CURSE_OFFSET++,len3+i+1,"]\n");
	mvprintw(GLOBAL_CURSE_OFFSET++,0,"\n");

	if(countDown > 0)
	{
		if(SUCCESS)
			attron(COLOR_PAIR(5));
		else 
			attron(COLOR_PAIR(6));
		attron(A_BOLD);
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"%s\n",INFOMSG);
		attroff(A_BOLD);
		attron(COLOR_PAIR(1));
	}
	else 
	{
		GLOBAL_CURSE_OFFSET++;
	}


	attron(COLOR_PAIR(4));
	attron(A_BOLD);
	mvprintw(GLOBAL_CURSE_OFFSET++,0,
		"[%c] - Exit | [%c] - Try kill selected pid | [%c,%c,%c,%c] - io/id/classic/custom view | [%c] - sort by mem | [%c] - sort by cpu | [%c] - default sort(PID)",
		QUIT_KEY,KILL_KEY,IO_KEY,ID_KEY,CLASSIC_KEY,CUSTOM_KEY,MEMSORT_KEY,CPUSORT_KEY,NORMALSORT_KEY);
	attron(COLOR_PAIR(1));
	attroff(A_BOLD);



	mvprintw(GLOBAL_CURSE_OFFSET++,0,"  \n");


}

void print_art()
{
	
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"			 ____  ____                  \n");
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"			|  _ \\/ ___|       _     _   \n");
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"			| |_) \\___ \\     _| |_ _| |_ \n");
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"			|  __/ ___) |   |_   _|_   _|\n");
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"			|_|   |____/      |_|   |_|  \n");
		

		mvprintw(GLOBAL_CURSE_OFFSET++,0,"\n");

}


/*odstampaj prvi red u outputu vodeci racuna o formatiranju i tome gde staviti znak | */
void print_header(char ** buffer, int buflength)
{
	char final[2048];
	final[0] = '\0';
	for(int i = 0; i < buflength; i++)
		{
			if 		(strcmp(buffer[i],"PID") == 0)			sprintf(final + strlen(final),"%-*s %s",formatvals[0],"PID",tabbord);
			else if (strcmp(buffer[i],"NAME") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[1],"NAME",tabbord);
			else if (strcmp(buffer[i],"STATE") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[2],"STATE",tabbord);
			else if (strcmp(buffer[i],"PPID") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[3],"PPID",tabbord);
			else if (strcmp(buffer[i],"TTY") == 0) 			sprintf(final + strlen(final),"%-*s %s",formatvals[4],"TTY",tabbord);
			else if (strcmp(buffer[i],"UTIME") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[5],"UTIME",tabbord);
			else if (strcmp(buffer[i],"STIME") == 0)  		sprintf(final + strlen(final),"%-*s %s",formatvals[6],"STIME",tabbord);
			else if (strcmp(buffer[i],"PRIO") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[7],"PRIO",tabbord);
			else if (strcmp(buffer[i],"NICE") == 0)  		sprintf(final + strlen(final),"%-*s %s",formatvals[8],"NICE",tabbord);
			else if (strcmp(buffer[i],"THREADNO") == 0) 	sprintf(final + strlen(final),"%-*s %s",formatvals[9],"THREADNO",tabbord);
			else if (strcmp(buffer[i],"VIRT") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[10],"VIRT",tabbord);
			else if (strcmp(buffer[i],"RES") == 0) 			sprintf(final + strlen(final),"%-*s %s",formatvals[11],"RES",tabbord);
			else if (strcmp(buffer[i],"OWNER") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[12],"OWNER",tabbord);
			else if (strcmp(buffer[i],"MEM%") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[13],"MEM%",tabbord);
			else if (strcmp(buffer[i],"CPU%") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[14],"CPU%",tabbord);
			else if (strcmp(buffer[i],"START") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[15],"START",tabbord);
			else if (strcmp(buffer[i],"SID") == 0) 			sprintf(final + strlen(final),"%-*s %s",formatvals[16],"SID",tabbord);
			else if (strcmp(buffer[i],"PGRP") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[17],"PGRP",tabbord);
			else if (strcmp(buffer[i],"C_WRITE") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[18],"C_WRITE",tabbord);
			else if (strcmp(buffer[i],"IO_READ") == 0) 		sprintf(final + strlen(final),"%-*s %s",formatvals[19],"IO_READ",tabbord);
			else if (strcmp(buffer[i],"IO_WRITE") == 0) 	sprintf(final + strlen(final),"%-*s %s",formatvals[20],"IO_WRITE",tabbord);
			else if (strcmp(buffer[i],"IO_READ/s") == 0) 	sprintf(final + strlen(final),"%-*s %s",formatvals[21],"IO_READ/s",tabbord);
			else if (strcmp(buffer[i],"IO_WRITE/s") == 0) 	sprintf(final + strlen(final),"%-*s %s",formatvals[22],"IO_WRITE/s",tabbord);
		}
		mvprintw(GLOBAL_CURSE_OFFSET++,0,"%s\n",final);
		printequals(buffer,buflength);
		
}

/* odstampaj podatke o procesu, vodeci racuna o formatiranju */
void print_data(char ** buffer, int buflength, PROCESS_LL * start)
{
		char final[2048];
		final[0] = '\0';
		for(int i = 0; i < buflength; i++)
		{
			if 		(strcmp(buffer[i],"PID") == 0)	
			{	
				PROCESSINFO t = start->info;
				sprintf( final + strlen(final),"%-*d ",formatvals[0],start->info.pid);
			}
			else if (strcmp(buffer[i],"NAME") == 0) 		sprintf( final + strlen(final),"%-*s %s",formatvals[1],start->info.name, tabbord);
			else if (strcmp(buffer[i],"STATE") == 0) 		sprintf( final + strlen(final),"%-*c %s",formatvals[2],start->info.state,tabbord);
			else if (strcmp(buffer[i],"PPID") == 0) 		sprintf( final + strlen(final),"%-*d %s",formatvals[3],start->info.ppid,tabbord);
			else if (strcmp(buffer[i],"TTY") == 0) 			sprintf( final + strlen(final),"%-*s %s",formatvals[4],start->info.ttyname,tabbord);
			else if (strcmp(buffer[i],"UTIME") == 0) 		sprintf( final + strlen(final),"%-*d %s",formatvals[5],start->info.utime,tabbord);
			else if (strcmp(buffer[i],"STIME") == 0)  		sprintf( final + strlen(final),"%-*d %s",formatvals[6],start->info.stime,tabbord);
			else if (strcmp(buffer[i],"PRIO") == 0) 		sprintf( final + strlen(final),"%-*ld %s",formatvals[7],start->info.prio,tabbord);
			else if (strcmp(buffer[i],"NICE") == 0)  		sprintf( final + strlen(final),"%-*ld %s",formatvals[8],start->info.nice,tabbord);
			else if (strcmp(buffer[i],"THREADNO") == 0) 	sprintf( final + strlen(final),"%-*ld %s",formatvals[9],start->info.threadno,tabbord);
			else if (strcmp(buffer[i],"VIRT") == 0) 		
			{
				sprintf( final + strlen(final),"%-*lu %s",formatvals[10],(start->info.virt)/1024,tabbord); //kilobajti
		


			}
			else if (strcmp(buffer[i],"RES") == 0) 			
			{
				sprintf( final + strlen(final),"%-*ld %s",formatvals[11],start->info.res * (pgsz/1024),tabbord); // konvertuj stranice u kb*/
			}
			else if (strcmp(buffer[i],"OWNER") == 0) 		sprintf( final + strlen(final),"%-*s %s",formatvals[12],start->info.user,tabbord);
			else if (strcmp(buffer[i],"MEM%") == 0) 		sprintf( final + strlen(final),"%-*.2lf %s",formatvals[13],((double)(start->info.res * (pgsz/1024))/memtotal) * 100,tabbord);
			else if (strcmp(buffer[i],"CPU%") == 0)
			{
				unsigned long cur = start->cpuinfo.utime_cur + start->cpuinfo.stime_cur;
				unsigned long prev = start->cpuinfo.utime_prev + start->cpuinfo.stime_prev;
				sprintf( final + strlen(final),"%-*.2lf %s",formatvals[14],((double)(cur-prev)/clock_ticks_ps) * 100, tabbord);

			}
				
			else if (strcmp(buffer[i],"START") == 0) 		
			{
				struct tm s;
				getcurrenttime(&s);
				if(s.tm_year != start->info.start_struct.tm_year)
					sprintf( final + strlen(final),"%-*d %s",formatvals[15],start->info.start_struct.tm_year + 1900,tabbord);
				else if(s.tm_mon == start->info.start_struct.tm_mon && s.tm_mday == start->info.start_struct.tm_mday)
					sprintf( final + strlen(final),"%s%d:%s%d %s",
						start->info.start_struct.tm_hour < 10 ? "0" : "",start->info.start_struct.tm_hour,
						start->info.start_struct.tm_min < 10 ? "0" : "" ,start->info.start_struct.tm_min,
						tabbord);
				else
				{
					sprintf( final + strlen(final),"%s%d%s %s",months[start->info.start_struct.tm_mon],start->info.start_struct.tm_mday,start->info.start_struct.tm_mday < 10 ? " " : "" ,tabbord);
				}
			}
			else if (strcmp(buffer[i],"SID") == 0) sprintf( final + strlen(final),"%-*d %s",formatvals[16],start->info.sid,tabbord);
			else if (strcmp(buffer[i],"PGRP") == 0) sprintf( final + strlen(final),"%-*d %s",formatvals[17],start->info.pgrp,tabbord);
			else if (strcmp(buffer[i],"C_WRITE") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.cancelled_write_bytes,start->ioinfo.io_blocked,0);
				sprintf( final + strlen(final),"%-*s %s",formatvals[18], br, tabbord);
			}
			else if (strcmp(buffer[i],"IO_READ") == 0) 
			{

				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.read_bytes_cur,start->ioinfo.io_blocked,0);
				sprintf( final + strlen(final),"%-*s %s",formatvals[19], br, tabbord);
			}
			else if (strcmp(buffer[i],"IO_WRITE") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.write_bytes_cur,start->ioinfo.io_blocked,0);
				sprintf( final + strlen(final),"%-*s %s",formatvals[20], br, tabbord);
			}
			else if (strcmp(buffer[i],"IO_READ/s") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.read_bytes_cur - start->ioinfo.read_bytes_prev,start->ioinfo.io_blocked,1);
				sprintf( final + strlen(final),"%-*s %s",formatvals[21], br, tabbord);
			}
			else if (strcmp(buffer[i],"IO_WRITE/s") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.write_bytes_cur - start->ioinfo.write_bytes_prev,start->ioinfo.io_blocked,1);
				sprintf( final + strlen(final),"%-*s %s",formatvals[22], br, tabbord);
			}
		}
		
		add_final(final,start->info.pid);

}



void displayScreen(char ** fbuf, int fno, char ** dformats, int dformatno)
{
	clear();
	/*if(!NOCOUNTDOWN)
		countDown--;*/
	GLOBAL_CURSE_OFFSET = 0;
	print_art();

	print_stats();
	
	if(fno != 0)
	{
		print_header(fbuf,fno); // menjaju offset
	}
	else 
	{
		print_header(dformats, dformatno); // menjaju offset
	}

	for(int i = start_pspp, pos = 0; i< y - GLOBAL_CURSE_OFFSET + start_pspp && i < fps_size; i++,pos++)
	{
		if(i == selectedLine)
			attron(COLOR_PAIR(3));
			mvprintw(pos + GLOBAL_CURSE_OFFSET,0,"%s\n",fps[i].mesg);
		if(i == selectedLine)
			attron(COLOR_PAIR(1));
	}
}






void 
criticalSection
(int * pid_args, int pno, char ** ubuffer, int uno, char ** nbuffer, int nno,int fno, char ** fbuffer, char ** default_formats, int default_format_no,int all)
{
	//all == 1 ? sve se refreshuje : samo se procesna lista refreshuje
		if(all)
			countDown--;
    	clear_and_reset_array(&fps, &fps_size);
        findprocs(&head, pid_args, pno, ubuffer, uno,nbuffer,nno);
        if(memSort) sortmem(&head);
        else if(cpuSort) sortCPU(&head);
        else if(normalSort) sortPID(&head); 

        formatvals[14] = 6; //cpu% 
		start = head;
		

		if(fno != 0)
			getprintno(fbuffer,fno);
		else 
			getprintno(default_formats,default_format_no);
		
		if(all)
		{
			getuptime();
			getmeminfo(&memtotal, &memfree, &memavailable,&swaptotal,&swapfree);
			cpu_percent = readSystemCPUTime();
		}
		

		while(start != NULL)
		{
			//pokupiti mrtve procese
			char _buf[1024];
			snprintf(_buf,sizeof(_buf),"/proc/%d",start->info.pid);
			if(stat(_buf,NULL) != 0 && errno == ENOENT)
			{
				PROCESS_LL * temp = start->next;
				removeElement(&head, start->info.pid);
				start = temp;
				continue;


			}
			if(fno != 0)
			{
				print_data(fbuffer,fno,start);
			}
			else 
			{
				print_data(default_formats,default_format_no,start);

			}
			start = start->next;
		}
}
int main(int argc, char ** argv)
{
	int use_custom = 0;
	
	int current = 1000;
	memSort = 0;
	normalSort = 1;
	cpuSort = 0;
	jFormat = 0;
	ioFormat = 0;
	
	snprintf(INFOMSG,sizeof(INFOMSG),"");
	long long diff = 0;
    fps = NULL;
    fps_size = 0;
	selectedLine = 0;
	GLOBAL_CURSE_OFFSET = 0;
	
	cpu_percent = readSystemCPUTime();
	curse_init();
	getbtime();
	clock_ticks_ps = sysconf(_SC_CLK_TCK);
	pgsz = sysconf(_SC_PAGESIZE);

	getuptime();
	getmeminfo(&memtotal, &memfree, &memavailable,&swaptotal,&swapfree);
	cpu_percent = readSystemCPUTime();

	//pripremi regex koji ce biti potreban svim modulima
    if (compile_regex() != 0) {
		freeList(head);
		free(fps);
		regfree(&regex);
		endwin();
        fprintf(stderr, "regex compilation failed\n");
        return EXIT_FAILURE;
    }

	


	int pid_count = 0;
	int pid_args[32767];
	
	
	//opcije koje ce se pojaviti ako se ne unesu eksplicitno polja(upravo onim redom kojim su definisana)
	char *default_formats[23] = 
	{"PID","STATE","PPID","TTY","PRIO","NICE",
	"VIRT","RES","OWNER", "MEM%", "CPU%","START","NAME"};
    int default_format_no = 13;

    //sve podrzane informacije o procesima
	char *formats[] = 		  
	{"PID","NAME","STATE","PPID","TTY","UTIME","STIME","PRIO","NICE","THREADNO",
	"VIRT","RES","OWNER", "MEM%", "CPU%","START","SID","PGRP","C_WRITE","IO_READ","IO_WRITE","IO_READ/s","IO_WRITE/s"};
	int format_no = 23;
	
	for(int i = 0; i < format_no; i++)
	{
		formatvals[i] = strlen(formats[i]);
	}


	//uneti kratke i duge opcije, parsirati ih i konacno proveriti validnost unetih podataka
	struct option f = {"-f","--format",NULL,0};
	struct option p = {"-p","--pid",NULL,0};
	struct option u = {"-u","--user",NULL,0};
	struct option n = {"-n","--name",NULL,0};
	
	struct arg_parse a = {{&f,&p,&u,&n},4};
	parseopts(argc,argv,&a);
	

	sanitycheck(f.buffer,f.no, formats, format_no,
				p.buffer,p.no,
				u.buffer,u.no);



	char *saved_custom[256];
	int saved_custom_length = 0;

	if(f.no == 0)
	{
		use_custom = 0;
		clscFormat = 1;
		customFormat = 0;
	}
	else 
	{
		use_custom = 1;
		for(int i = 0; i < f.no; i++)
			saved_custom[i] = f.buffer[i];
		saved_custom_length = f.no;
		
		clscFormat = 0;
		customFormat = 1;
	}
	for(int i = 0; i < p.no; i++)
	{
		pid_args[i] = atoi(p.buffer[i]);
	}

	
	head = NULL;
	long long beforeCritical = getTimeInMilliseconds();
	int first_pspp = 1;
	countDown = 0;
	pthread_t cursorUpdateThread;
	pthread_create(&cursorUpdateThread,NULL,cursorUpdate,NULL);
	while(1)
	{ 
		nosleep:
		formatvals[15] = 5;
		long long afterCritical = getTimeInMilliseconds();
		

		if(first_pspp || afterCritical - beforeCritical >= 1000)
	    {
	    	first_pspp = 0;
	    	beforeCritical = afterCritical;
			criticalSection(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,1);
	    }
		if(selectedLine > fps_size - 1)
			selectedLine = fps_size - 1;

		if(fps_size == 1) selectedLine = 0;
		NOCOUNTDOWN = 0;
	    displayScreen(f.buffer, f.no, default_formats, default_format_no);
	    NOCOUNTDOWN = 1;

	    
		long long beforeInput = getTimeInMilliseconds();
		int ch = getch();

		if(ch == QUIT_KEY)
		{
			endwin();
			pthread_cancel(cursorUpdateThread);
			freeList(head);
			free(fps);
			regfree(&regex);
			exit(EXIT_SUCCESS);
		}
		if(ch == KEY_DOWN && fps_size > 0)
		{
			if(selectedLine < fps_size - 1)
			{
				selectedLine++;
				if(selectedLine == y + start_pspp - GLOBAL_CURSE_OFFSET)
					start_pspp++;
				displayScreen(f.buffer, f.no, default_formats, default_format_no);
			}
		}
		else if(ch == KEY_UP)
		{
			if(selectedLine > 0 && fps_size > 0)
			{
				if(cursy == 0 && start_pspp > 0)
				{
					start_pspp--;
				}
				selectedLine--;
				displayScreen(f.buffer, f.no, default_formats, default_format_no);
			}

		}
		else if(ch == KEY_RESIZE)
		{
			displayScreen(f.buffer, f.no, default_formats, default_format_no);
		}
		else if (ch == 10)
		{
			endwin();
			printf("pid: %d\n",fps[selectedLine].pid);
			pthread_cancel(cursorUpdateThread);
			freeList(head);
			free(fps);
			regfree(&regex);
			exit(EXIT_SUCCESS);
		}
		else if(ch == KILL_KEY && fps_size > 0)
		{
			if(kill(fps[selectedLine].pid,SIGTERM) == 0)
			{
				snprintf(INFOMSG,sizeof(INFOMSG),"\tKilled [%d]\t",fps[selectedLine].pid);
				SUCCESS = 1;
				countDown = 5;
			}
			else 
			{
				snprintf(INFOMSG,sizeof(INFOMSG),"\tCouldn't kill [%d]: %s\t",fps[selectedLine].pid,strerror(errno));
				SUCCESS = 0;
				countDown = 5;
			}
			findprocs(&head, pid_args, p.no, u.buffer, u.no,n.buffer,n.no);
			start = head;
			criticalSection(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,0);
			displayScreen(f.buffer, f.no, default_formats, default_format_no);
		}
		else if(ch == ID_KEY && fps_size > 0 && !jFormat)
		{
			jFormat = 1;
			ioFormat = 0;
			clscFormat = 0;
			customFormat = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tSwitched to id mode\t",fps[selectedLine].pid);
			SUCCESS = 1;
			countDown = 5;
			char ** cl;
			int *j;
			if(f.no == 0) 
			{
				cl = default_formats;
				j = &(default_format_no);
			}
			else 
			{
				cl = f.buffer;
				j = &(f.no);

			}
			for(int i = 0; i < *j; i++)
			{
				cl[i] = NULL;
			}
			cl[0] = "PID";
			cl[1] = "PPID";
			cl[2] = "PGRP";
			cl[3] = "SID";
			cl[4] = "NAME";
			*j = 5;
			criticalSection(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,0);
			displayScreen(f.buffer, f.no, default_formats, default_format_no);

		}
		else if(ch == IO_KEY && fps_size > 0 && !ioFormat)
		{
			ioFormat = 1;
			jFormat = 0;
			clscFormat = 0;
			customFormat = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tSwitched to io mode\t",fps[selectedLine].pid);
			SUCCESS = 1;
			countDown = 5;
			
			char ** cl;
			int *j;
			if(f.no == 0) 
			{
				cl = default_formats;
				j = &(default_format_no);
			}
			else 
			{
				cl = f.buffer;
				j = &(f.no);

			}
			for(int i = 0; i < *j; i++)
			{
				cl[i] = NULL;
			}
			cl[0] = "PID";
			cl[1] = "IO_READ/s";
			cl[2] = "IO_WRITE/s";
			cl[3] = "IO_READ";
			cl[4] = "IO_WRITE";
			cl[5] = "C_WRITE";
			cl[6] = "NAME";
			*j = 7;
			criticalSection(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,0);
			displayScreen(f.buffer, f.no, default_formats, default_format_no);

		}
		else if(ch == CLASSIC_KEY && fps_size > 0 && !clscFormat)
		{
			classic:
			ioFormat = 0;
			jFormat = 0;
			clscFormat = 1;
			customFormat = 0;
			
			snprintf(INFOMSG,sizeof(INFOMSG),"\tSwitched to classic mode\t",fps[selectedLine].pid);
			SUCCESS = 1;
			countDown = 5;

			char ** cl;
			int *j;
			if(f.no == 0) 
			{
				cl = default_formats;
				j = &(default_format_no);
			}
			else 
			{
				cl = f.buffer;
				j = &(f.no);

			}
			for(int i = 0; i < *j; i++)
			{
				cl[i] = NULL;
			}
			cl[0] = "PID";
			cl[1] = "STATE";
			cl[2] = "PPID";
			cl[3] = "TTY";
			cl[4] = "PRIO";
			cl[5] = "NICE";
			cl[6] = "VIRT";
			cl[7] = "RES";
			cl[8] = "OWNER";
			cl[9] = "MEM%";
			cl[10] = "CPU%";
			cl[11] = "START";
			cl[12] = "NAME";
			*j = 13;
			criticalSection(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,0);
			displayScreen(f.buffer, f.no, default_formats, default_format_no);

		}
		else if(ch == MEMSORT_KEY && fps_size > 0 && !memSort)
		{
			memSort = 1;
			normalSort = 0;
			cpuSort = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by mem\t");
			SUCCESS = 1;
			countDown = 5;

		}
		else if(ch == CPUSORT_KEY && fps_size > 0 && !cpuSort)
		{
			cpuSort = 1;
			normalSort = 0;
			memSort = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by CPU%\t");
			SUCCESS = 1;
			countDown = 5;
		}
		else if(ch == NORMALSORT_KEY && fps_size > 0 && (cpuSort || memSort))
		{
			cpuSort = 0;
			normalSort = 1;
			memSort = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by default(PID)\t");
			SUCCESS = 1;
			countDown = 5;
		}
		else if(ch == CUSTOM_KEY && fps_size > 0 && !customFormat)
		{
			if(!use_custom)
			{
				if(!clscFormat)
				{
					ch = CLASSIC_KEY;
					goto classic;
				}

			}
			else 
			{
				customFormat = 1;
				clscFormat = 0;
				ioFormat = 0;
				jFormat = 0;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tSwitched back to custom format mode\t",fps[selectedLine].pid);
				SUCCESS = 1;
				countDown = 5;
				
				for(int i = 0; i < saved_custom_length; i++)
				{
					f.buffer[i] = saved_custom[i];
				}
				f.no = saved_custom_length;
				
				criticalSection(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,0);
				displayScreen(f.buffer, f.no, default_formats, default_format_no);
			}
		}
		long long elapsed = getTimeInMilliseconds() - beforeInput;
		

		current -= elapsed;
		if(current <= 20)
			current = 1000;
		timeout(current);

	}
	

	freeList(head);
	free(fps);
	regfree(&regex);
	exit(EXIT_SUCCESS);

}