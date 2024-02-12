#include <ncurses.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include "structures.h"
#include "infocollect.h"
#include "helper.h"
#include "parseopts.h"
#include "configreader.h"
#include "dynamic_array_manager.h"
#include "systeminfo.h"

#ifdef FORMAT_TABLE
	#define tabbord (i == buflength - 1 ? "|" : "")
	#define tabstart (i == 0 ? "|" : "")
#else 
	#define tabbord (i == buflength - 1 ? " " : "")
	#define tabstart (i == 0 ? " " : "")
#endif

#define INFOMSG_POSITION 15
#define FILTERMSG_POSITION 16

#define LAG 10 //ms
#define REFRESH_RATE 1000 //ms


static char INFOMSG[65];
static int GLOBAL_CURSE_OFFSET;
static int LIST_START;
static int countDown;
static int SUCCESS;
static int FILTERING;
static int selectedLine;
static int x,y,cursx,cursy;
static int start_pspp;
static double cpu_percent;


PROCESS_LL * head;
PROCESS_LL * start;
int formatvals[24];
long clock_ticks_ps;
long pgsz;
long long memtotal, memfree, memavailable,swaptotal,swapfree;
long btime;
unsigned long uptime;
unsigned long long allprocs;
unsigned long long countprocs;
char NAME_FILTER[256];
char MOUNT_POINT[256];


static void printhelpmenu()
{
	clear();
	int helpline = 0;
	getmaxyx(stdscr,y,x);
	attron(COLOR_PAIR(1));
	attron(A_REVERSE);
	mvprintw(helpline++,(x-12)/2,"ps_pp manual");
	attroff(A_REVERSE);


	attron(A_BOLD);
	mvprintw(helpline++,0,"USAGE");
	attroff(A_BOLD);
	mvprintw(helpline++,4,"ps_pp [options]");
	
	helpline++;
	attron(A_BOLD);
	mvprintw(helpline++,0,"DESCRIPTION");
	attroff(A_BOLD);
	mvprintw(helpline++,4,"ps_pp is a command-line utility to display process information.");
	helpline++;
	attron(A_BOLD);
	mvprintw(helpline++,0,"OPTIONS");
	attroff(A_BOLD);
	mvprintw(helpline++,4,"-p, --pid <username>");
	mvprintw(helpline++,8,"Display processes matching given PID(s).");
	mvprintw(helpline++,4,"-u, --user <username>");
	mvprintw(helpline++,8,"Display processes owned by the specified user(s).");
	mvprintw(helpline++,4,"-f, --format <format_string>");
	mvprintw(helpline++,8,"Specify the output format using the provided format option(s)");
	mvprintw(helpline++,4,"-n, --name <regex>");
	mvprintw(helpline++,8,"Display information about processes whose command name match the pattern(s).");
	helpline++;
	attron(A_BOLD);
	mvprintw(helpline++,0,"CURRENT KEY BINDINGS");
	attroff(A_BOLD);

	mvprintw(helpline++,4,"[F4] - Name filter mode");
	mvprintw(helpline++,4,"[%c] - Quit program",QUIT_KEY);
	mvprintw(helpline++,4,"[%c] - Send SIGTERM signal",KILL_KEY);
	mvprintw(helpline++,4,"[%c] - Send SIGKILL signal",KILLKILL_KEY);
	mvprintw(helpline++,4,"[%c] - Suspend/unsuspend process",TOGGLESUSPEND_KEY);
	mvprintw(helpline++,4,"[%c] - Switch to IO mode",IO_KEY);
	mvprintw(helpline++,4,"[%c] - Switch to ID mode",ID_KEY);
	mvprintw(helpline++,4,"[%c] - Switch to Classic mode",CLASSIC_KEY);
	mvprintw(helpline++,4,"[%c] - Switch to custom defined format",CUSTOM_KEY);
	mvprintw(helpline++,4,"[%c] - Return to the start of list",BACK_KEY);
	mvprintw(helpline++,4,"[%c] - Jump to the end of list",END_KEY);
	mvprintw(helpline++,4,"[%c] - Increment nice",NICEPLUS_KEY);
	mvprintw(helpline++,4,"[%c] - Decrement nice",NICEMINUS_KEY);
	mvprintw(helpline++,4,"[%c] - Sort by default(PID)",NORMALSORT_KEY);
	mvprintw(helpline++,4,"[%c] - Sort by CPU%%",CPUSORT_KEY);
	mvprintw(helpline++,4,"[%c] - Sort by MEM%%",MEMSORT_KEY);
	mvprintw(helpline++,4,"[%c] - Sort by priority",PRIOSORT_KEY);
	mvprintw(helpline++,4,"[%c] - Display this help menu",HELP_KEY);
	
	helpline++;
}


static void curse_init()
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
	timeout(REFRESH_RATE);
    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        exit(EXIT_FAILURE);
    }
	start_color();
	init_color(COLOR_BLACK,COLOR_BG[0],COLOR_BG[1],COLOR_BG[2]); 						//COLOR_BG
	init_color(COLOR_BLUE,COLOR_TEXT[0],COLOR_TEXT[1],COLOR_TEXT[2]); 					//COLOR_TEXT
	init_color(COLOR_RED,COLOR_PRCNT_BAR[0],COLOR_PRCNT_BAR[1],COLOR_PRCNT_BAR[2]); 	//COLOR_PRCNT_BAR
	init_color(COLOR_CYAN,COLOR_SELECTPROC[0],COLOR_SELECTPROC[1],COLOR_SELECTPROC[2]); //COLOR_SELECTPROC
	init_color(COLOR_MAGENTA,COLOR_INFOTEXT[0],COLOR_INFOTEXT[1],COLOR_INFOTEXT[2]); 	//COLOR_INFOTEXT
	init_color(COLOR_GREEN,COLOR_INFO_SUC[0],COLOR_INFO_SUC[1],COLOR_INFO_SUC[2]); 		//COLOR_INFO_SUC
	init_color(COLOR_YELLOW,COLOR_INFO_ERR[0],COLOR_INFO_ERR[1],COLOR_INFO_ERR[2]); 	//COLOR_INFO_ERR


	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_RED, COLOR_BLACK);
	init_pair(3,COLOR_BLACK,COLOR_CYAN);
	init_pair(4,COLOR_BLACK,COLOR_MAGENTA);
	init_pair(5,COLOR_WHITE,COLOR_GREEN);
	init_pair(6,COLOR_WHITE,COLOR_YELLOW);
	attron(COLOR_PAIR(1));

}





static void handle_io(char * dest, size_t destsize, long long target, int blocked, int pers)
{
	double vd;
	char * s = ""; 
	if(target > 1048576LL)
	{
		vd = (double)target / 1048576LL;
		s = "M";
	}
	else if (target > 1024LL)
	{
		vd = (double)target / 1024LL;
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
		snprintf(dest, destsize, "%.1lf%sB%s", vd,s, pers ? "/s" : "");
	}

}

/* odstampaj podatke o zauzetosti operativne memorije, iskoriscenost procesora,
*  iskoriscenost swap prostora, uptime sistema, i totalan broj procesa */

static void prettyprint(char * s, int len)
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


static void print_stats()
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


	char procbuf1[2048];
	int proclen1 = x > 45 ? snprintf(procbuf1,sizeof(procbuf1),"Processes discovered on the system: %llu\n",allprocs) : 
							snprintf(procbuf1,sizeof(procbuf1),"P: %llu\n",allprocs);
	
	prettyprint(procbuf1,proclen1);

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
	if(FILTERING)
	{
		char buf[300];
		attron(COLOR_PAIR(5));
		attron(A_BOLD);
		int len = snprintf(buf,sizeof(buf),"Name:%s%s\n",strlen(NAME_FILTER) > 0 ?  " " : "",NAME_FILTER);
		mvprintw(FILTERMSG_POSITION,0,"%s",buf);
		attron(A_BLINK);
		mvprintw(FILTERMSG_POSITION,len-1,"|");
		attroff(A_BLINK);
		attroff(A_BOLD);
		attron(COLOR_PAIR(1));
	}
	else 
	{
		for(int i = 0; i < 2048 && i < x; i++)
			mvprintw(FILTERMSG_POSITION,i," ");
	}

}

static void print_art()
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
static void print_header(char ** buffer, int buflength, char ** formats, int format_no, int * printno_export)
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
	mvprintw(GLOBAL_CURSE_OFFSET,0,"%s\n",final);
	for(int k = strlen(final); k < x && k < 2048; k++)
	{
		mvprintw(GLOBAL_CURSE_OFFSET,k," ");
	}
	++GLOBAL_CURSE_OFFSET;
	attron(COLOR_PAIR(1));
	
	*printno_export = printno;
	
}

/* odstampaj podatke o procesu, vodeci racuna o formatiranju */
static void collect_data(char ** buffer, int buflength, PROCESS_LL * start)
{
		char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
		char final[2048];
		final[0] = '\0';
		for(int i = 0; i < buflength; i++)
		{
			if (strcmp(buffer[i],"PID") == 0)	
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
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[10],start->info.virt_display,tabbord);
			}
			else if (strcmp(buffer[i],"RES") == 0) 			
			{
				//snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[11],start->info.res * (pgsz/1024),tabbord); // konvertuj stranice u kb*/
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[11],start->info.res_display,tabbord);
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
			else if (strcmp(buffer[i],"SHR") == 0)
			{
				//snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lld %s",tabstart,formatvals[23], start->info.shr, tabbord);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[23],start->info.shr_display,tabbord);
			}
		}
		
		add_final(final,start->info.pid,start->info.state);

}

static void print_upper_menu_and_mod_offset(char ** fbuf, int fno, char ** dformats, int dformatno, char ** formats, int format_no, int * printno_export)
{

	GLOBAL_CURSE_OFFSET = 6;
	print_stats(); //menjaju offset
	
	if(fno != 0)
	{
		print_header(fbuf,fno,formats,format_no,printno_export); // menjaju offset
	}
	else 
	{
		print_header(dformats, dformatno,formats,format_no,printno_export); // menjaju offset
	}
	LIST_START = GLOBAL_CURSE_OFFSET;

}


static void refreshList()
{
	char optimization[2048];
	if(FILTERING)
		attron(A_DIM);
	for(int i = start_pspp, pos = 0; i< y - LIST_START + start_pspp && i < fps_size; i++,pos++)
	{
		snprintf(optimization,x+1,"%s",fps[i].mesg);
		for(int j = strlen(fps[i].mesg); j < x && j < 2048; j++)
		{
			snprintf(optimization + j,x," ");
		} 
		if(i == selectedLine)
			attron(COLOR_PAIR(3));
			mvprintw(pos + LIST_START,0,"%s\n",optimization/*fps[i].mesg*/);
		if(i == selectedLine)
			attron(COLOR_PAIR(1));
	}
	attroff(A_DIM);
}


static void displayScreen(char ** fbuf, int fno, char ** dformats, int dformatno, char ** formats, int format_no, int * printno_export)
{
	print_upper_menu_and_mod_offset(fbuf,fno,dformats,dformatno,formats,format_no,printno_export);
	refreshList();
}



static void sortRefresh(int memSort, int cpuSort, int normalSort, int prioSort)
{
    if(memSort) sortmem(&head);
    else if(cpuSort) sortCPU(&head);
    else if(normalSort) sortPID(&head); 
    else if(prioSort) sortPrio(&head);
}


static void updateSysinfo()
{
    countDown--;
	getuptime();
	getmeminfo(&memtotal, &memfree, &memavailable,&swaptotal,&swapfree);
	cpu_percent = readSystemCPUTime();	
}


/*ponovo pokupi podatke u spregnutu listu, obrisi mrtve procese i procese koji nam vise ne trebaju za trenutnu selekciju. */
static void updateListInternal
(int * pid_args, int pno, char ** ubuffer, int uno, char ** nbuffer, int nno,int fno, char ** fbuffer, char ** default_formats, int default_format_no, char ** formats, int format_no,
int memSort, int cpuSort, int prioSort, int normalSort)
{
	for(int i = 0; i < format_no; i++)
	{
		formatvals[i] = strlen(formats[i]);
	}
	formatvals[13] = 7; //mem% 
	formatvals[14] = 6; //cpu% 
	formatvals[15] = 5; //start

	clear_and_reset_array(&fps, &fps_size);
    findprocs(&head, pid_args, pno, ubuffer, uno,nbuffer,nno,fbuffer,fno);
	sortRefresh(memSort,cpuSort,normalSort,prioSort);

	start = head;
	while(start != NULL)
	{
		//pokupiti mrtve procese
		char _buf[1024];
		snprintf(_buf,sizeof(_buf),"%s/%d",MOUNT_POINT,start->info.pid);
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


static void 
criticalSection
(int * pid_args, int pno, char ** ubuffer, int uno, char ** nbuffer, int nno,int fno, char ** fbuffer, char ** default_formats, int default_format_no, char ** formats, int format_no,
int memSort, int cpuSort, int prioSort, int normalSort)
{
		updateSysinfo();
    	updateListInternal(pid_args, pno, ubuffer, uno, nbuffer, nno, fno, fbuffer, default_formats, default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
}


int main(int argc, char ** argv)
{
	get_mount_point();
	NAME_FILTER[0] = '\0';
	FILTERING = 0;
	int jFormat;
	int ioFormat;
	int clscFormat;
	int customFormat;
	int printno_export;
	int memSort;
	int normalSort;
	int cpuSort;
	int prioSort;
	int pid_args[32767];
    //sve podrzane informacije o procesima
	char *formats[] = 		  
	{"PID","NAME","STATE","PPID","TTY","UTIME","STIME","PRIO","NICE","THREADNO",
	"VIRT","RES","OWNER", "MEM%", "CPU%","START","SID","PGRP","C_WRITE","IO_READ","IO_WRITE","IO_READ/s","IO_WRITE/s","SHR"};
	const int format_no = 24;

	//opcije koje ce se pojaviti ako se ne unesu eksplicitno polja(prikaz je upravo onim redom kojim su definisana)
	char *default_formats[24] = 
	{"PID","STATE","PPID","TTY","PRIO","NICE", "SHR",
	"VIRT","RES","OWNER", "MEM%", "CPU%","START","NAME"};
    int default_format_no = 14;
	
	
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
	
	int current = REFRESH_RATE;
	memSort = 0;
	normalSort = 1;
	cpuSort = 0;
	prioSort = 0;
	jFormat = 0;
	ioFormat = 0;
	
    fps = NULL;
    fps_size = 0;
	selectedLine = 0;
	
	GLOBAL_CURSE_OFFSET = 6;
	LIST_START = 0;
	int HELP_MODE = 0;
	
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

		sortRefresh(memSort,cpuSort,normalSort,prioSort);
		long long afterCritical = getTimeInMilliseconds();


		if(first_pspp || afterCritical - beforeCritical >= REFRESH_RATE)
	    {
	    	first_pspp = 0;
	    	beforeCritical = afterCritical;

			criticalSection(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats,format_no,memSort,cpuSort,prioSort,normalSort);

			if(!HELP_MODE)
			{
				clear();
				print_art();

				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
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
			if(FILTERING)
			{
				char buf[300];
				attron(COLOR_PAIR(5));
				attron(A_BOLD);
				int len = snprintf(buf,sizeof(buf),"Name:%s%s\n",strlen(NAME_FILTER) > 0 ?  " " : "",NAME_FILTER);
				mvprintw(FILTERMSG_POSITION,0,"%s",buf);
				attron(A_BLINK);
				mvprintw(FILTERMSG_POSITION,len-1,"|");
				attroff(A_BLINK);
				attroff(A_BOLD);
				attron(COLOR_PAIR(1));
			}
			else 
			{
				for(int i = 0; i < 2048 && i < x; i++)
					mvprintw(FILTERMSG_POSITION,i," ");
			}
		}
		
		long long beforeInput = getTimeInMilliseconds();
		if(!HELP_MODE)
			refreshList();
		



		int ch = getch();
		if(ch == KEY_F(4) && !HELP_MODE)
		{
			FILTERING = !FILTERING;
		}
		move(selectedLine - start_pspp,0);
		
		getyx(stdscr,cursy,cursx);
		if(!FILTERING)
		{
			if(ch == QUIT_KEY)
			{
				endwin();
				freeList(head);
				free(fps);
				exit(EXIT_SUCCESS);
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
					displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
				}	

			}
			else if (ch == HELP_KEY)
			{
				if(!HELP_MODE)
				{
					HELP_MODE = 1;
					printhelpmenu();
				}
				else 
				{
					HELP_MODE = 0;
					clear();
					print_art();
					displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
				}
				
			}
	        else if (ch == KEY_MOUSE && fps_size > 0 && !HELP_MODE) 
	        {
	            MEVENT event;
	            if (getmouse(&event) == OK) {
	                if (event.bstate & BUTTON1_PRESSED && event.y >= LIST_START /*&& event.x <= printno_export*/) {
	                	selectedLine = start_pspp + event.y - LIST_START;
	                }
	            }
	            displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
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
			else if(ch == KILL_KEY && fps_size > 0 && !HELP_MODE)
			{
				if(kill(fps[selectedLine].pid,SIGTERM) == 0)
				{
					snprintf(INFOMSG,sizeof(INFOMSG),"\tKilled [%d]\t",fps[selectedLine].pid);
					SUCCESS = 1;
					countDown = 5;
					updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
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
					displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
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
				
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

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
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

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
				cl[6] = "SHR";
				cl[7] = "VIRT";
				cl[8] = "RES";
				cl[9] = "OWNER";
				cl[10] = "MEM%";
				cl[11] = "CPU%";
				cl[12] = "START";
				cl[13] = "NAME";
				*j = 14;
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

			}
			else if(ch == MEMSORT_KEY && fps_size > 0 && !memSort && !HELP_MODE)
			{
				memSort = 1;
				normalSort = 0;
				cpuSort = 0;
				prioSort = 0;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by mem\t");
				SUCCESS = 1;
				countDown = 5;
				sortmem(&head);
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

			}
			else if(ch == CPUSORT_KEY && fps_size > 0 && !cpuSort && !HELP_MODE)
			{
				cpuSort = 1;
				normalSort = 0;
				memSort = 0;
				prioSort = 0;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by CPU%%\t");
				SUCCESS = 1;
				countDown = 5;
				sortCPU(&head);
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
			}
			else if(ch == NORMALSORT_KEY && fps_size > 0 && !normalSort && !HELP_MODE)
			{
				cpuSort = 0;
				normalSort = 1;
				memSort = 0;
				prioSort = 0;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by default(PID)\t");
				SUCCESS = 1;
				countDown = 5;
				sortPID(&head);
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

			}
			else if(ch == PRIOSORT_KEY && fps_size > 0 && !prioSort && !HELP_MODE)
			{
				cpuSort = 0;
				normalSort = 0;
				memSort = 0;
				prioSort = 1;
				
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by priority\t");
				SUCCESS = 1;
				countDown = 5;
				
				sortPrio(&head);
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

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
					
					updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
					displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
				}
			}
			else if(ch == BACK_KEY && fps_size > 0 && selectedLine > 0 && !HELP_MODE)
			{
				start_pspp = 0;
				selectedLine = 0;
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
				

				snprintf(INFOMSG,sizeof(INFOMSG),"\tReturned to top      ");
				SUCCESS = 1;
				countDown = 5;
			}
			else if(ch == END_KEY && fps_size > 0 && selectedLine < fps_size - 1 && !HELP_MODE)
			{
				selectedLine = fps_size - 1;
				if(fps_size > y - LIST_START)
					start_pspp = fps_size -(y - LIST_START);
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
				

				snprintf(INFOMSG,sizeof(INFOMSG),"\tJumped to end      ");
				SUCCESS = 1;
				countDown = 5;
			}
			else if((ch == NICEPLUS_KEY || ch == NICEMINUS_KEY) && !HELP_MODE && fps_size > 0)
			{
				errno = 0;
				int prio = getpriority(PRIO_PROCESS,fps[selectedLine].pid);
				if(prio == -1 && errno != 0)
				{
					snprintf(INFOMSG,sizeof(INFOMSG),"\tError reading current nice value for [%d]: %s",fps[selectedLine].pid,strerror(errno));
					SUCCESS = 0;
					countDown = 5;
				}
				else 
				{
					if(setpriority(PRIO_PROCESS,fps[selectedLine].pid,ch == NICEPLUS_KEY ? prio + 1 : prio -1) == 0)
					{
						snprintf(INFOMSG,sizeof(INFOMSG),"\t%s nice for [%d]",ch == NICEPLUS_KEY ? "Incremented" : "Decremented",fps[selectedLine].pid);
						SUCCESS = 1;
						countDown = 5;
						updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
						displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
					}
					else 
					{
						snprintf(INFOMSG,sizeof(INFOMSG),"\tCouldn't %s nice for [%d]: %s",ch == NICEPLUS_KEY ? "increment" : "decrement",fps[selectedLine].pid, strerror(errno));
						SUCCESS = 0;
						countDown = 5;
					}
				}

			}
			else if(ch == TOGGLESUSPEND_KEY && fps_size > 0 && !HELP_MODE)
			{
				int sig = fps[selectedLine].s == 'T' ? SIGCONT : SIGSTOP;
				if(kill(fps[selectedLine].pid,sig) == 0)
				{
					snprintf(INFOMSG,sizeof(INFOMSG),"%suspended [%d]",fps[selectedLine].s == 'T' ? "Uns" : "S",fps[selectedLine].pid);
					SUCCESS = 1;
					countDown = 5;
					updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
					displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
				}
				else 
				{
					snprintf(INFOMSG,sizeof(INFOMSG),"Couldn't %suspend [%d]: %s",fps[selectedLine].s == 'T' ? "un" : "s",fps[selectedLine].pid,strerror(errno));
					SUCCESS = 0;
					countDown = 5;
				}


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
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
			}	

		}
		else
		{
			//(ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '?' || ch == '!' || ch == '-' || ch == '/' || ch == ' '
			if(isprint(ch))
			{
				snprintf(NAME_FILTER + strlen(NAME_FILTER),sizeof(NAME_FILTER) - strlen(NAME_FILTER),"%c",ch);
				clear();
				print_art();
				selectedLine = 0;
				start_pspp = 0;
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
			}
			else if(ch == KEY_BACKSPACE && strlen(NAME_FILTER) > 0)
			{
				NAME_FILTER[strlen(NAME_FILTER) - 1] = '\0';
				clear();
				print_art();
				selectedLine = 0;
				start_pspp = 0;
				updateListInternal(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,memSort,cpuSort,prioSort,normalSort);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
			}
		}
		long long elapsed = getTimeInMilliseconds() - beforeInput;


		current -= elapsed;

		if(current <= 0)
			current = REFRESH_RATE;
	

		
		timeout(current);
		refresh();
		napms(LAG);

	}
	

	freeList(head);
	free(fps);
	exit(EXIT_SUCCESS);

}