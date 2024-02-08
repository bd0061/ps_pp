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
#include <errno.h>
#include <sys/stat.h>
#include "configreader.h"


#define tabbord (i == buflength - 1 ? "|" : "")
#define tabstart (i == 0 ? "|" : "")
#define INFOMSG_POSITION 15




PROCESS_LL * head;
PROCESS_LL * start;

int LIST_START;
int memSort;
int normalSort;
int cpuSort;
int refre;
int HELP_MODE;
int countDown;

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

int printno_export;


void printhelpmenu()
{
	clear();
	int helpline = 0;
	HELP_MODE = 1;
	getmaxyx(stdscr,y,x);
	attron(COLOR_PAIR(1));
	attron(A_REVERSE);
	mvprintw(helpline++,(x-12)/2,"ps_pp manual");
	attroff(A_REVERSE);



	mvprintw(helpline++,0,"USAGE");
	mvprintw(helpline++,4,"ps_pp [options]");
	
	helpline++;
	mvprintw(helpline++,0,"DESCRIPTION");
	mvprintw(helpline++,4,"ps_pp is a command-line utility to display process information.");
	helpline++;
	mvprintw(helpline++,0,"OPTIONS");
	mvprintw(helpline++,4,"-p, --pid <username>");
	mvprintw(helpline++,8,"Display processes matching given PID(s).");
	mvprintw(helpline++,4,"-u, --user <username>");
	mvprintw(helpline++,8,"Display processes owned by the specified user(s).");
	mvprintw(helpline++,4,"-f, --format <format_string>");
	mvprintw(helpline++,8,"Specify the output format using the provided format option(s)");
	mvprintw(helpline++,4,"-n, --name <regex>");
	mvprintw(helpline++,8,"Display information about processes whose command name match the pattern(s).");
	helpline++;
	mvprintw(helpline++,0,"CURRENT KEY BINDINGS");


	mvprintw(helpline++,4,"[%c] - Quit program",QUIT_KEY);
	mvprintw(helpline++,4,"[%c] - Send SIGTERM signal",KILL_KEY);
	mvprintw(helpline++,4,"[%c] - Switch to IO mode",IO_KEY);
	mvprintw(helpline++,4,"[%c] - Switch to ID mode",ID_KEY);
	mvprintw(helpline++,4,"[%c] - Switch to Classic mode",CLASSIC_KEY);
	mvprintw(helpline++,4,"[%c] - Switch to custom defined format",CUSTOM_KEY);
	mvprintw(helpline++,4,"[%c] - Return to the start of list",BACK_KEY);
	mvprintw(helpline++,4,"[%c] - Jump to the end of list",END_KEY);
	mvprintw(helpline++,4,"[%c] - Sort by default(PID)",NORMALSORT_KEY);
	mvprintw(helpline++,4,"[%c] - Sort by CPU%%",CPUSORT_KEY);
	mvprintw(helpline++,4,"[%c] - Sort by MEM%%",MEMSORT_KEY);
	mvprintw(helpline++,4,"[%c] - Display this help menu",HELP_KEY);
	
	helpline++;



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
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
	mouseinterval(0);
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
	init_pair(4,COLOR_BLACK,COLOR_MAGENTA);
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
unsigned long long countprocs_prev;

int CPU_FIRST = 1;


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
		endwin();
        perror("Error opening /proc/stat");
        exit(EXIT_FAILURE);
    }

    char line[256];
    if (fgets(line, sizeof(line), stat_file) == NULL) {
		freeList(head);
		free(fps);
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

void prettyprint(char * s, int len)
{
	for(int j = 0; j < 2048 && j < x; j++)
	{
		attron(A_DIM);
		mvprintw(GLOBAL_CURSE_OFFSET,j,"-");
		attroff(A_DIM);
	}

	mvprintw(GLOBAL_CURSE_OFFSET,x/2 - len/2,"%s",s);

	for(int j = x/2 + len/2; j < 2048 && j < x; j++)
	{
		attron(A_DIM);
		mvprintw(GLOBAL_CURSE_OFFSET,j,"-");
		attroff(A_DIM);
	}

	GLOBAL_CURSE_OFFSET++;
}


void print_stats()
{
 	int days, hours, minutes, seconds;
	convertseconds(uptime, &days, &hours, &minutes, &seconds);
	

	char upbuf[2048];
	int uplen =  x > 45 ? 
	snprintf(upbuf,sizeof(upbuf),"Uptime: %d day%s, %d hour%s, %d minute%s, %d second%s\n",days, days != 1 ? "s" : "", hours, hours != 1 ? "s" : "", 
		minutes, minutes != 1 ? "s" : "",seconds, seconds != 1 ? "s" : "") 
	:
	snprintf(upbuf,sizeof(upbuf),"UT: %dD %dH %dMIN %dS\n",days,hours,minutes,seconds);
	
	prettyprint(upbuf,uplen);

	/*mvprintw(GLOBAL_CURSE_OFFSET++,0,"Uptime: %d day%s, %d hour%s, %d minute%s, %d second%s\n",days, days != 1 ? "s" : "", hours, hours != 1 ? "s" : "", 
		minutes, minutes != 1 ? "s" : "",seconds, seconds != 1 ? "s" : "");*/

	char procbuf1[2048];
	int proclen1 = snprintf(procbuf1,sizeof(procbuf1),"Processes on the system: %llu\n",allprocs);
	
	prettyprint(procbuf1,proclen1);
	
	//mvprintw(GLOBAL_CURSE_OFFSET++,0,"Processes on the system: %llu\n",allprocs);
	

	char procbuf2[2048];
	int proclen2 = snprintf(procbuf2,sizeof(procbuf2),"Of which displayed: %llu\n\n",countprocs);
	prettyprint(procbuf2,proclen2);
	
	int barsize = x/2;
	if(barsize>50)	barsize=50;
	int len = x/2 - (barsize + 2)/2;


	if(cpu_percent >= 100.0) cpu_percent = 100.0; //kompenzacija za moguce kasnjenje kupljenja informacija

	int onebar = 0;
	double scaled_cpu_percent = (cpu_percent * barsize) / 100;
	
	char CPUbuf[1024];
	int len1 = snprintf(CPUbuf,sizeof(CPUbuf),"CPU: %.1lf%%",cpu_percent);
	
	for(int i = 0; i  < 2048 && i < x; i++) mvprintw(GLOBAL_CURSE_OFFSET,i, " ");	


	mvprintw(GLOBAL_CURSE_OFFSET++,(x-len1)/2,"%s",CPUbuf);

	for(int i = 0; i  < 2048 && i < x; i++) mvprintw(GLOBAL_CURSE_OFFSET,i, " ");	
	
	attron(A_BOLD);
	mvprintw(GLOBAL_CURSE_OFFSET,len,"/");
	attroff(A_BOLD);
	int i;
	
	for(i = 0; i < barsize; i++)
	{
		if(!onebar && scaled_cpu_percent > 0 && (int)scaled_cpu_percent == 0)
		{
			attron(COLOR_PAIR(2));
			onebar = 1;
		}
		else if(i < (int)scaled_cpu_percent)
		{
			attron(COLOR_PAIR(2));
		}
		else
		{
			attron(COLOR_PAIR(1));
			attron(A_DIM);
		}
		mvprintw(GLOBAL_CURSE_OFFSET,len+i+1,"/");
		attroff(A_DIM);
	}
	attron(COLOR_PAIR(1));
	attroff(A_DIM);
	attron(A_BOLD);
	mvprintw(GLOBAL_CURSE_OFFSET++,len+i+1,"/\n");
	attroff(A_BOLD);


	onebar = 0;
	double percent_memory = (double)(memtotal-memavailable)/memtotal * 100;
	//real prcnt : 100 = new_prcnt : barsize
   
	double scaled_percent_memory = (percent_memory * barsize)/100;
	
	char membuf[1024];
	int len2 = snprintf(membuf,sizeof(membuf),"Mem: %.2lfGB/%.2lfGB", (double)(memtotal-memavailable)/1000000,(double)memtotal/1000000);

	for(int i = 0; i  < 2048 && i < x; i++) mvprintw(GLOBAL_CURSE_OFFSET,i, " ");	

	mvprintw(GLOBAL_CURSE_OFFSET++,(x-len2)/2,"%s",membuf);
	
	for(int i = 0; i  < 2048 && i < x; i++) mvprintw(GLOBAL_CURSE_OFFSET,i, " ");	

	attron(A_BOLD);
	mvprintw(GLOBAL_CURSE_OFFSET,len,"/");
	attroff(A_BOLD);
	
	i = 0;
	
	for(i = 0; i < barsize; i++)
	{
		if(!onebar && scaled_percent_memory > 0 && (int)scaled_percent_memory == 0)
		{
			onebar = 1;
			attron(COLOR_PAIR(2));
		}
	    else if(i < (int)scaled_percent_memory)
	    {
	    	attron(COLOR_PAIR(2));
	    }
	    else 
	    {
	    	attron(COLOR_PAIR(1));
	    	attron(A_DIM);
	       
	    }
	    mvprintw(GLOBAL_CURSE_OFFSET,len+i+1,"/");
	    attroff(A_DIM);
	}

	attron(COLOR_PAIR(1));
	attroff(A_DIM);
	attron(A_BOLD);
	mvprintw(GLOBAL_CURSE_OFFSET++,len+i+1,"/\n");
	attroff(A_BOLD);




	double percent_swap = (double)(swaptotal-swapfree)/swaptotal * 100;
	//real prcnt : 100 = new_prcnt : barsize

	double scaled_percent_swap = (percent_swap * barsize) / 100;

	char swapbuf[1024];

	int len3 = snprintf(swapbuf,sizeof(swapbuf),
		"Swap: %.2lf%s/%.2lfGB", 
		swaptotal - swapfree < swaptotal/8 && swaptotal - swapfree < 1000000LL ? (double)(swaptotal-swapfree)/1000 : (double)(swaptotal-swapfree)/1000000,
		swaptotal - swapfree < swaptotal/8 ? "KB": "GB",
		(double)swaptotal/1000000);
	

	for(int i = 0; i  < 2048 && i < x; i++) mvprintw(GLOBAL_CURSE_OFFSET,i, " ");	
	mvprintw(GLOBAL_CURSE_OFFSET++,(x-len3)/2,"%s",swapbuf);
	for(int i = 0; i  < 2048 && i < x; i++) mvprintw(GLOBAL_CURSE_OFFSET,i, " ");	
	attron(A_BOLD);
	mvprintw(GLOBAL_CURSE_OFFSET,len,"/");
	attroff(A_BOLD);
	onebar = 0;
	
	i=0;
	for(i = 0; i < barsize; i++)
	{
		//hak da se bar jedna crtica ispise za vrlo male vrednosti
		if(!onebar && scaled_percent_swap > 0 && (int)scaled_percent_swap == 0)
		{
			onebar = 1;
			attron(COLOR_PAIR(2));
		}
	    else if(i < (int)scaled_percent_swap)
	    {
	    	attron(COLOR_PAIR(2));
	    }
	    else 
	    {
	    	attron(COLOR_PAIR(1));
	    	attron(A_DIM);
	        
	    }
	    mvprintw(GLOBAL_CURSE_OFFSET,len+i+1,"/");
	    attroff(A_DIM);
	}

	attron(COLOR_PAIR(1));
	attroff(A_DIM);
	attron(A_BOLD);
	mvprintw(GLOBAL_CURSE_OFFSET++,len+i+1,"/\n");
	attroff(A_BOLD);
	
	GLOBAL_CURSE_OFFSET++;

	if(countDown > 0)
	{
		if(SUCCESS)
			attron(COLOR_PAIR(5));
		else 
			attron(COLOR_PAIR(6));
		attron(A_BOLD);
		mvprintw(INFOMSG_POSITION,0,"%s\n",INFOMSG);
		attroff(A_BOLD);
		attron(COLOR_PAIR(1));
	}
	else 
	{
		for(int i = 0; i < 2048 && i < x; i++)
			mvprintw(INFOMSG_POSITION,i," ");
	}

	/*GLOBAL_CURSE_OFFSET++;
	attron(COLOR_PAIR(4));
	attron(A_BOLD);
	mvprintw(GLOBAL_CURSE_OFFSET++,0,
		"[%c]-Exit|[%c]-Try kill selected pid|[%c,%c,%c,%c]-io/id/classic/custom view|[%c]-sort by mem|[%c]-sort by cpu|[%c]-default sort(PID)|[%c]-return to top",
		QUIT_KEY,KILL_KEY,IO_KEY,ID_KEY,CLASSIC_KEY,CUSTOM_KEY,MEMSORT_KEY,CPUSORT_KEY,NORMALSORT_KEY,BACK_KEY);
	attron(COLOR_PAIR(1));
	attroff(A_BOLD);



	mvprintw(GLOBAL_CURSE_OFFSET++,0,"  \n");*/


}

void print_art()
{
	for(int i = 0; i < 6; i++)
	{
		for(int j = 0; j < x && j < 2048; j++)
		{
			mvprintw(i,j," ");
		}
	}
	if(x > 45)
	{
		mvprintw(0,x/2 - 16, "    ____  _____                \n");
		mvprintw(1,x/2 - 16, "   / __ \\/ ___/       __    __ \n");
		mvprintw(2,x/2 - 16, "  / /_/ /\\__ \\     __/ /___/ /_\n");
		mvprintw(3,x/2 - 16, " / ____/___/ /    /_  __/_  __/\n");
		mvprintw(4,x/2 - 16, "/_/    /____/      /_/   /_/   \n");
	}
	else 
	{
		mvprintw(2,x/2 - 8," _  __        \n");
		mvprintw(3,x/2 - 8,"|_)(_   _|__|_\n");
		mvprintw(4,x/2 - 8,"|  __)   |  |  \n");
	}
	

	mvprintw(5,0,"\n");

}

/*odstampaj prvi red u outputu vodeci racuna o formatiranju i tome gde staviti znak | */
void print_header(char ** buffer, int buflength, char ** formats, int format_no)
{
	int printno = 0;
	char final[2048];
	final[0] = '\0';
	for(int i = 0; i < buflength; i++)
	{
		for(int j = 0; j < format_no; j++)
		{
			if(strcmp(buffer[i],formats[j]) == 0)
			{



				printno += snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",(i == 0 ? " " : ""),formatvals[j],buffer[i],(i == buflength - 1 ? " " : ""));
			}
		}
	}
	

	GLOBAL_CURSE_OFFSET++;
	attron(COLOR_PAIR(4));
	mvprintw(GLOBAL_CURSE_OFFSET++,0,"%s\n",final);
	attron(COLOR_PAIR(1));
	
	/*for(int i = 0; i < 2048; i++)
	{
		mvprintw(GLOBAL_CURSE_OFFSET,i,"%c", i < printno && i < x ? '=' : ' ');
		mvprintw(GLOBAL_CURSE_OFFSET-2,i,"%c",i < printno && i < x ? '=' : ' ');
	}*/

	printno_export = printno;
	//GLOBAL_CURSE_OFFSET++;
	
}

/* odstampaj podatke o procesu, vodeci racuna o formatiranju */
void collect_data(char ** buffer, int buflength, PROCESS_LL * start)
{
		char final[2048];
		final[0] = '\0';
		for(int i = 0; i < buflength; i++)
		{
			if 		(strcmp(buffer[i],"PID") == 0)	
			{	
				PROCESSINFO t = start->info;
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*d %s",tabstart,formatvals[0],start->info.pid,tabbord);
			}
			else if (strcmp(buffer[i],"NAME") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[1],start->info.name, tabbord);
			else if (strcmp(buffer[i],"STATE") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*c %s",tabstart,formatvals[2],start->info.state,tabbord);
			else if (strcmp(buffer[i],"PPID") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*d %s",tabstart,formatvals[3],start->info.ppid,tabbord);
			else if (strcmp(buffer[i],"TTY") == 0) 			snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[4],start->info.ttyname,tabbord);
			else if (strcmp(buffer[i],"UTIME") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[5],start->info.utime,tabbord);
			else if (strcmp(buffer[i],"STIME") == 0)  		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[6],start->info.stime,tabbord);
			else if (strcmp(buffer[i],"PRIO") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[7],start->info.prio,tabbord);
			else if (strcmp(buffer[i],"NICE") == 0)  		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[8],start->info.nice,tabbord);
			else if (strcmp(buffer[i],"THREADNO") == 0) 	snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[9],start->info.threadno,tabbord);
			else if (strcmp(buffer[i],"VIRT") == 0) 		
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%-*lu %s",formatvals[10],(start->info.virt)/1024,tabbord); //kilobajti
		


			}
			else if (strcmp(buffer[i],"RES") == 0) 			
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[11],start->info.res * (pgsz/1024),tabbord); // konvertuj stranice u kb*/
			}
			else if (strcmp(buffer[i],"OWNER") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[12],start->info.user,tabbord);
			else if (strcmp(buffer[i],"MEM%") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*.2lf %s",tabstart,formatvals[13],((double)(start->info.res * (pgsz/1024))/memtotal) * 100,tabbord);
			else if (strcmp(buffer[i],"CPU%") == 0)
			{
				unsigned long cur = start->cpuinfo.utime_cur + start->cpuinfo.stime_cur;
				unsigned long prev = start->cpuinfo.utime_prev + start->cpuinfo.stime_prev;
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*.2lf %s",tabstart,formatvals[14],((double)(cur-prev)/clock_ticks_ps) * 100, tabbord);

			}
				
			else if (strcmp(buffer[i],"START") == 0) 		
			{
				struct tm s;
				getcurrenttime(&s);
				if(s.tm_year != start->info.start_struct.tm_year)
					snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*d %s",tabstart,formatvals[15],start->info.start_struct.tm_year + 1900,tabbord);
				else if(s.tm_mon == start->info.start_struct.tm_mon && s.tm_mday == start->info.start_struct.tm_mday)
					snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%s%d:%s%d %s",
						tabstart,
						start->info.start_struct.tm_hour < 10 ? "0" : "",start->info.start_struct.tm_hour,
						start->info.start_struct.tm_min < 10 ? "0" : "" ,start->info.start_struct.tm_min,
						tabbord);
				else
				{
					snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%s%d%s %s",tabstart,months[start->info.start_struct.tm_mon],start->info.start_struct.tm_mday,start->info.start_struct.tm_mday < 10 ? " " : "" ,tabbord);
				}
			}
			else if (strcmp(buffer[i],"SID") == 0) sprintf( final + strlen(final),"%s%-*d %s",tabstart,formatvals[16],start->info.sid,tabbord);
			else if (strcmp(buffer[i],"PGRP") == 0) sprintf( final + strlen(final),"%s%-*d %s",tabstart,formatvals[17],start->info.pgrp,tabbord);
			else if (strcmp(buffer[i],"C_WRITE") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.cancelled_write_bytes,start->ioinfo.io_blocked,0);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[18], br, tabbord);
			}
			else if (strcmp(buffer[i],"IO_READ") == 0) 
			{

				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.read_bytes_cur,start->ioinfo.io_blocked,0);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[19], br, tabbord);
			}
			else if (strcmp(buffer[i],"IO_WRITE") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.write_bytes_cur,start->ioinfo.io_blocked,0);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[20], br, tabbord);
			}
			else if (strcmp(buffer[i],"IO_READ/s") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.read_bytes_cur - start->ioinfo.read_bytes_prev,start->ioinfo.io_blocked,1);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[21], br, tabbord);
			}
			else if (strcmp(buffer[i],"IO_WRITE/s") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.write_bytes_cur - start->ioinfo.write_bytes_prev,start->ioinfo.io_blocked,1);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[22], br, tabbord);
			}
		}
		
		add_final(final,start->info.pid);

}

void print_upper_menu_and_mod_offset(char ** fbuf, int fno, char ** dformats, int dformatno, char ** formats, int format_no)
{

	GLOBAL_CURSE_OFFSET = 6;

	print_stats(); //menjaju offset
	
	if(fno != 0)
	{
		print_header(fbuf,fno,formats,format_no); // menjaju offset
	}
	else 
	{
		print_header(dformats, dformatno,formats,format_no); // menjaju offset
	}
	LIST_START = GLOBAL_CURSE_OFFSET;

}


void refreshList()
{
	char optimization[2048];
	for(int i = start_pspp, pos = 0; i< y - LIST_START + start_pspp && i < fps_size; i++,pos++)
	{
		snprintf(optimization,x,"%s",fps[i].mesg);
		if(i == selectedLine)
			attron(COLOR_PAIR(3));
			mvprintw(pos + LIST_START,0,"%s\n",optimization/*fps[i].mesg*/);
		if(i == selectedLine)
			attron(COLOR_PAIR(1));
	}
}


void displayScreen(char ** fbuf, int fno, char ** dformats, int dformatno, char ** formats, int format_no)
{
	print_upper_menu_and_mod_offset(fbuf,fno,dformats,dformatno,formats,format_no);
	refreshList();
}



void sortRefresh()
{
    if(memSort) sortmem(&head);
    else if(cpuSort) sortCPU(&head);
    else if(normalSort) sortPID(&head); 
}


void updateSysinfo()
{
    countDown--;
	getuptime();
	getmeminfo(&memtotal, &memfree, &memavailable,&swaptotal,&swapfree);
	cpu_percent = readSystemCPUTime();	
}



void reformat(int fno, char ** fbuffer, char ** default_formats, int default_format_no)
{
	
	clear_and_reset_array(&fps, &fps_size);
	start = head;
	while(start != head)
	{
		if(fno != 0)
		{
			collect_data(fbuffer,fno,start);
		}
		else 
		{
			collect_data(default_formats,default_format_no,start);

		}
		start = start->next;

	}

}


void updateListInternal(int * pid_args, int pno, char ** ubuffer, int uno, char ** nbuffer, int nno,int fno, char ** fbuffer, char ** default_formats, int default_format_no, char ** formats, int format_no)
{
	static int first_internal = 1;
	countprocs_prev = countprocs;

	for(int i = 0; i < format_no; i++)
	{
		formatvals[i] = strlen(formats[i]);
	}
	formatvals[13] = 7; //mem% 
	formatvals[14] = 6; //cpu% 
	formatvals[15] = 5; //start

	clear_and_reset_array(&fps, &fps_size);
    findprocs(&head, pid_args, pno, ubuffer, uno,nbuffer,nno);
	sortRefresh();
    if(!first_internal && countprocs_prev != countprocs && countprocs <= y-GLOBAL_CURSE_OFFSET && selectedLine == fps_size - 1)
    {
 		/*refreshList();
    	clear();
    	print_art();
    	refre = 1;*/
    }
    first_internal = 0;
	start = head;
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
			collect_data(fbuffer,fno,start);
		}
		else 
		{
			collect_data(default_formats,default_format_no,start);

		}
		start = start->next;
	}

}


void 
criticalSection
(int * pid_args, int pno, char ** ubuffer, int uno, char ** nbuffer, int nno,int fno, char ** fbuffer, char ** default_formats, int default_format_no, char ** formats, int format_no)
{
		updateSysinfo();
    	updateListInternal(pid_args, pno, ubuffer, uno, nbuffer, nno, fno, fbuffer, default_formats, default_format_no,formats, format_no);
}


int main(int argc, char ** argv)
{
	countprocs_prev = 0;
	int pid_args[32767];
	refre = 0;
    //sve podrzane informacije o procesima
	char *formats[] = 		  
	{"PID","NAME","STATE","PPID","TTY","UTIME","STIME","PRIO","NICE","THREADNO",
	"VIRT","RES","OWNER", "MEM%", "CPU%","START","SID","PGRP","C_WRITE","IO_READ","IO_WRITE","IO_READ/s","IO_WRITE/s"};
	const int format_no = 23;

	//opcije koje ce se pojaviti ako se ne unesu eksplicitno polja(prikaz je upravo onim redom kojim su definisana)
	char *default_formats[23] = 
	{"PID","STATE","PPID","TTY","PRIO","NICE",
	"VIRT","RES","OWNER", "MEM%", "CPU%","START","NAME"};
    int default_format_no = 13;
	
	
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
	parseopts(argv,&a);
	

	sanitycheck(f.buffer,f.no, formats, format_no,
				p.buffer,p.no,
				u.buffer,u.no);
	
	int use_custom = 0;
	
	int current = 1000;
	memSort = 0;
	normalSort = 1;
	cpuSort = 0;
	jFormat = 0;
	ioFormat = 0;
	
	snprintf(INFOMSG,sizeof(INFOMSG)," ");
    fps = NULL;
    fps_size = 0;
	selectedLine = 0;
	
	GLOBAL_CURSE_OFFSET = 6;
	LIST_START = 0;
	HELP_MODE = 0;
	
	cpu_percent = readSystemCPUTime();
	curse_init();
	getbtime();
	clock_ticks_ps = sysconf(_SC_CLK_TCK);
	pgsz = sysconf(_SC_PAGESIZE);

	getuptime();
	getmeminfo(&memtotal, &memfree, &memavailable,&swaptotal,&swapfree);
	cpu_percent = readSystemCPUTime();

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
	
	getmaxyx(stdscr,y,x);
	move(selectedLine - start_pspp,0);
	getyx(stdscr,cursy,cursx);

	int fps_saved;
	print_art();
	while(1)
	{
		getmaxyx(stdscr,y,x);
		fps_saved = fps_size;

		sortRefresh();
		long long afterCritical = getTimeInMilliseconds();


		if(first_pspp || afterCritical - beforeCritical >= 1000)
	    {
	    	first_pspp = 0;
	    	beforeCritical = afterCritical;

			criticalSection(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats,format_no);

			if(!HELP_MODE)
			{
				clear();
				print_art();

				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
			}
		}
		
		if (selectedLine < 0) selectedLine = 0;

		if(selectedLine != 0 && selectedLine > fps_size - 1)
			selectedLine = fps_size - 1;

		if(fps_size == 1) selectedLine = 0;
		
		if(!HELP_MODE)
		{
			if(countDown > 0)
			{
				if(SUCCESS)
					attron(COLOR_PAIR(5));
				else 
					attron(COLOR_PAIR(6));
				attron(A_BOLD);
				mvprintw(INFOMSG_POSITION,0,"%s\n",INFOMSG);
				attroff(A_BOLD);
				attron(COLOR_PAIR(1));
			}
			else 
			{
				for(int i = 0; i < 2048 && i < x; i++)
					mvprintw(INFOMSG_POSITION,i," ");
			}
		}

	    
		
		long long beforeInput = getTimeInMilliseconds();
		if(!HELP_MODE)
			refreshList();
		int ch = getch();
		
		//getmaxyx(stdscr,y,x);
		move(selectedLine - start_pspp,0);
		
		getyx(stdscr,cursy,cursx);
		if(ch == QUIT_KEY)
		{
			endwin();
			freeList(head);
			free(fps);
			exit(EXIT_SUCCESS);
		}
		else if (ch == HELP_KEY)
		{
			if(!HELP_MODE)
			{
				printhelpmenu();
			}
			else 
			{
				HELP_MODE = 0;
				clear();
				print_art();
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
			}
			
		}
        else if (ch == KEY_MOUSE && fps_size > 0 && !HELP_MODE) 
        {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_PRESSED && event.y >= LIST_START && event.x <= printno_export) {
                	selectedLine = start_pspp + event.y - LIST_START;
                }
            }
            displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
        }
		else if(ch == KEY_DOWN && fps_size > 0 && !HELP_MODE)
		{
			if(selectedLine < fps_size - 1)
			{
				selectedLine++;
				if(selectedLine == y + start_pspp - GLOBAL_CURSE_OFFSET)
					start_pspp++;
				refreshList();
			}
		}
		else if(ch == KEY_UP && !HELP_MODE)
		{
			if(selectedLine > 0 && fps_size > 0)
			{
				if(cursy == 0 && start_pspp > 0)
				{
					start_pspp--;
				}
				selectedLine--;
				refreshList();
			}

		}
		else if(ch == KEY_RESIZE)
		{
			if(HELP_MODE)
			{
				printhelpmenu();
			}
			else 
			{
				int _y,_x;
				getmaxyx(stdscr,_y,_x);
				if(fps_size > 0 && _y < y && y - LIST_START - 1 != 0 && selectedLine - start_pspp == y - LIST_START - 1)
				{
					start_pspp++;
				}
				x=_x;
				y=_y;
				print_art();
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
			}	

		}
		else if(ch == KILL_KEY && fps_size > 0 && !HELP_MODE)
		{
			if(kill(fps[selectedLine].pid,SIGTERM) == 0)
			{
				snprintf(INFOMSG,sizeof(INFOMSG),"\tKilled [%d]\t",fps[selectedLine].pid);
				SUCCESS = 1;
				countDown = 5;
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
				clear();
				if(selectedLine > 0 && fps_size > 0)
				{
					if(cursy == 0 && start_pspp > 0)
					{
						start_pspp--;
					}
					selectedLine--;
				}
				print_art();
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
			}
			else 
			{
				snprintf(INFOMSG,sizeof(INFOMSG),"\tCouldn't kill [%d]: %s\t",fps[selectedLine].pid,strerror(errno));
				SUCCESS = 0;
				countDown = 5;
			}
		}
		else if(ch == ID_KEY && fps_size > 0 && !jFormat && !HELP_MODE)
		{
			jFormat = 1;
			ioFormat = 0;
			clscFormat = 0;
			customFormat = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tSwitched to id mode\t");
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
			
			cl[0] = "PID";cl[1] = "PPID";cl[2] = "PGRP";cl[3] = "SID";cl[4] = "NAME";
			*j = 5;
			
			updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
			displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);

		}
		else if(ch == IO_KEY && fps_size > 0 && !ioFormat && !HELP_MODE)
		{
			ioFormat = 1;
			jFormat = 0;
			clscFormat = 0;
			customFormat = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tSwitched to io mode\t");
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
			updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
			displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);

		}
		else if(ch == CLASSIC_KEY && fps_size > 0 && !clscFormat && !HELP_MODE)
		{
			classic:
			ioFormat = 0;
			jFormat = 0;
			clscFormat = 1;
			customFormat = 0;
			
			snprintf(INFOMSG,sizeof(INFOMSG),"\tSwitched to classic mode\t");
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
			updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
			displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);

		}
		else if(ch == MEMSORT_KEY && fps_size > 0 && !memSort && !HELP_MODE)
		{
			memSort = 1;
			normalSort = 0;
			cpuSort = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by mem\t");
			SUCCESS = 1;
			countDown = 5;
			sortmem(&head);
			updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
			displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);

		}
		else if(ch == CPUSORT_KEY && fps_size > 0 && !cpuSort && !HELP_MODE)
		{
			cpuSort = 1;
			normalSort = 0;
			memSort = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by CPU%\t");
			SUCCESS = 1;
			countDown = 5;
			sortCPU(&head);
			updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
			displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
		}
		else if(ch == NORMALSORT_KEY && fps_size > 0 && (cpuSort || memSort) && !HELP_MODE)
		{
			cpuSort = 0;
			normalSort = 1;
			memSort = 0;
			snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by default(PID)\t");
			SUCCESS = 1;
			countDown = 5;
			sortPID(&head);
			updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
			displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);

		}
		else if(ch == CUSTOM_KEY && fps_size > 0 && !customFormat && !HELP_MODE)
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
				snprintf(INFOMSG,sizeof(INFOMSG),"\tSwitched back to custom format mode\t");
				SUCCESS = 1;
				countDown = 5;
				
				for(int i = 0; i < saved_custom_length; i++)
				{
					f.buffer[i] = saved_custom[i];
				}
				f.no = saved_custom_length;
				
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
			}
		}
		else if(ch == BACK_KEY && fps_size > 0 && selectedLine > 0 && !HELP_MODE)
		{
			start_pspp = 0;
			selectedLine = 0;
			updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
			displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
			

			snprintf(INFOMSG,sizeof(INFOMSG),"\tReturned to top      ");
			SUCCESS = 1;
			countDown = 5;
		}
		else if(ch == END_KEY && fps_size > 0 && selectedLine < fps_size - 1 && !HELP_MODE)
		{
			selectedLine = fps_size - 1;
			if(fps_size > y - LIST_START)
				start_pspp = fps_size -(y - LIST_START);
			updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no);
			displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no);
			

			snprintf(INFOMSG,sizeof(INFOMSG),"\tJumped to end      ");
			SUCCESS = 1;
			countDown = 5;
		}
		long long elapsed = getTimeInMilliseconds() - beforeInput;

		if(current <= 20)
			current = 1000;
	

		
		timeout(current);
		refresh();
		napms(10);

	}
	

	freeList(head);
	free(fps);
	exit(EXIT_SUCCESS);

}