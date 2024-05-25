#define _GNU_SOURCE
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
#include <sched.h>

#include "dynamic_array_manager.h"
#include "../../backend_api.h"

char INFOMSG[65];
int GLOBAL_CURSE_OFFSET;
int LIST_START;
int countDown;
int SUCCESS;
int FILTERING;
int selectedLine;
int x,y,cursx,cursy;
int start_pspp;


int formatvals[35];
long clock_ticks_ps;
long pgsz;
long btime;
unsigned long uptime;

char NAME_FILTER[256];


// inciijalizacija UI biblioteke ncurses
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

// stampanje pomocnog menija unutar programa
void printhelpmenu()
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
	mvprintw(helpline++,8,"Specify the output format using the provided format option(s).");
	mvprintw(helpline++,4,"-n, --name <name>");
	mvprintw(helpline++,8,"Display information about processes whose command name matches the string(s).");
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
	mvprintw(helpline++,4,"[%c] - Sort by starttime (oldest first)",TIMESORT_KEY_OLDEST);
	mvprintw(helpline++,4,"[%c] - Sort by starttime (newest first)",TIMESORT_KEY);
	mvprintw(helpline++,4,"[%c] - Display this help menu",HELP_KEY);
	
	helpline++;
}

//ispravan izbor jedinica u zavisnosti od velicine
void handle_io(char * dest, size_t destsize, long long target, int blocked, int pers)
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
		snprintf(dest,destsize,"%s", "N/A");

	}
	else 
	{
		snprintf(dest, destsize, "%.1lf%sB%s", vd,s, pers ? "/s" : "");
	}

}

// prateci '-' karakteri kod sistem info teksta
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

// stampanje info teksta iznad liste procesa
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


//stampanje logoa iznad sistem infa
void print_art()
{
	if(!NOLOGO)
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
}

/*odstampaj prvi red u outputu vodeci racuna o formatiranju i tome gde staviti znak | */
void print_header(char ** buffer, int buflength, char ** formats, int format_no, int * printno_export)
{
	int printno = 0;
	char final[2048];
	final[0] = '\0';
	for(int i = 0; i < buflength; i++)
	{
		for(int j = 0; j < format_no; j++)
		{
			if(strcasecmp(buffer[i],formats[j]) == 0)
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


// izvuci iz liste procesa sve informacije u formatirani string, i dodaj ga u filtriranu strukturu kao i pid i stanje za real-time pristup procesima
void collect_data(char ** buffer, int buflength, PROCESS_LL * start)
{
		char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
		char final[2048];
		final[0] = '\0';
		for(int i = 0; i < buflength; i++)
		{

			if (strcasecmp(buffer[i],"PID") == 0)	
			{	
				PROCESSINFO t = start->info;
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*d %s",tabstart,formatvals[0],start->info.pid,tabbord);
			}
			else if (strcasecmp(buffer[i],"NAME") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[1],start->info.name, tabbord);
			else if (strcasecmp(buffer[i],"STATE") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*c %s",tabstart,formatvals[2],start->info.state,tabbord);
			else if (strcasecmp(buffer[i],"PPID") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*d %s",tabstart,formatvals[3],start->info.ppid,tabbord);
			else if (strcasecmp(buffer[i],"TTY") == 0) 			snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[4],start->info.ttyname,tabbord);
			else if (strcasecmp(buffer[i],"UTIME") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[5],start->info.utime,tabbord);
			else if (strcasecmp(buffer[i],"STIME") == 0)  		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[6],start->info.stime,tabbord);
			else if (strcasecmp(buffer[i],"PRIO") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[7],start->info.prio,tabbord);
			else if (strcasecmp(buffer[i],"NICE") == 0)  		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[8],start->info.nice,tabbord);
			else if (strcasecmp(buffer[i],"THREADNO") == 0) 	snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[9],start->info.threadno,tabbord);
			else if (strcasecmp(buffer[i],"VIRT") == 0) 		
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[10],start->info.virt_display,tabbord);
			}
			else if (strcasecmp(buffer[i],"RES") == 0) 			
			{
				//snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[11],start->info.res * (pgsz/1024),tabbord); // konvertuj stranice u kb*/
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[11],start->info.res_display,tabbord);
			}
			else if (strcasecmp(buffer[i],"OWNER") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[12],start->info.user,tabbord);
			else if (strcasecmp(buffer[i],"MEM%") == 0) 		snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*.2lf %s",tabstart,formatvals[13],((double)(start->info.res * (pgsz/1024))/memtotal) * 100,tabbord);
			else if (strcasecmp(buffer[i],"CPU%") == 0)
			{
				unsigned long cur = start->cpuinfo.utime_cur + start->cpuinfo.stime_cur;
				unsigned long prev = start->cpuinfo.utime_prev + start->cpuinfo.stime_prev;
				double cpu_prcnt = ((double)(cur-prev)/clock_ticks_ps) * 100;
				if(cpu_prcnt > 100.0f) cpu_prcnt = 100.0f; // :)
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*.2lf %s",tabstart,formatvals[14],cpu_prcnt, tabbord);

			}
				
			else if (strcasecmp(buffer[i],"START") == 0) 		
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
			else if (strcasecmp(buffer[i],"SID") == 0) sprintf( final + strlen(final),"%s%-*d %s",tabstart,formatvals[16],start->info.sid,tabbord);
			else if (strcasecmp(buffer[i],"PGRP") == 0) sprintf( final + strlen(final),"%s%-*d %s",tabstart,formatvals[17],start->info.pgrp,tabbord);
			else if (strcasecmp(buffer[i],"C_WRITE") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.cancelled_write_bytes,start->ioinfo.io_blocked,0);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[18], br, tabbord);
			}
			else if (strcasecmp(buffer[i],"IO_READ") == 0) 
			{

				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.read_bytes_cur,start->ioinfo.io_blocked,0);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[19], br, tabbord);
			}
			else if (strcasecmp(buffer[i],"IO_WRITE") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.write_bytes_cur,start->ioinfo.io_blocked,0);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[20], br, tabbord);
			}
			else if (strcasecmp(buffer[i],"IO_READ/S") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.read_bytes_cur - start->ioinfo.read_bytes_prev,start->ioinfo.io_blocked,1);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[21], br, tabbord);
			}
			else if (strcasecmp(buffer[i],"IO_WRITE/S") == 0) 
			{
				char br[1024];
				handle_io(br,sizeof(br),start->ioinfo.write_bytes_cur - start->ioinfo.write_bytes_prev,start->ioinfo.io_blocked,1);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[22], br, tabbord);
			}
			else if (strcasecmp(buffer[i],"SHR") == 0)
			{
				//snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lld %s",tabstart,formatvals[23], start->info.shr, tabbord);
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[23],start->info.shr_display,tabbord);
			}
			else if (strcasecmp(buffer[i],"MINFLT") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[24],start->info.minflt,tabbord);
			}
			else if (strcasecmp(buffer[i],"CMINFLT") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[25],start->info.cminflt,tabbord);
			}
			else if (strcasecmp(buffer[i],"MAJFLT") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[26],start->info.majflt,tabbord);
			}
			else if (strcasecmp(buffer[i],"CMAJFLT") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[27],start->info.cmajflt,tabbord);
			}
			else if (strcasecmp(buffer[i],"CUTIME") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[28],start->info.cutime / clock_ticks_ps,tabbord);
			}
			else if (strcasecmp(buffer[i],"CSTIME") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*ld %s",tabstart,formatvals[29],start->info.cstime / clock_ticks_ps,tabbord);
			}
			else if (strcasecmp(buffer[i],"RESLIM") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*lu %s",tabstart,formatvals[30],start->info.reslim,tabbord);
			}
			else if (strcasecmp(buffer[i],"EXITSIG") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[31],strsignal(start->info.exitsig),tabbord);
			}
			else if (strcasecmp(buffer[i],"PROCNO") == 0)
			{
				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*d %s",tabstart,formatvals[32],start->info.procno,tabbord);
			}
			else if (strcasecmp(buffer[i],"RTPRIO") == 0)
			{
				if(start->info.rtprio != 0)
					snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*u %s",tabstart,formatvals[33],start->info.rtprio,tabbord);
				else 
					snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[33],"-",tabbord);
			}
			else if (strcasecmp(buffer[i],"POLICY") == 0)
			{
				char * schedstring;

				switch(start->info.policy)
				{
					case SCHED_RR:
					case SCHED_OTHER:
						schedstring = "RR";
						break;
					case SCHED_BATCH:
						schedstring = "BATCH";
						break;
					case SCHED_IDLE:
						schedstring = "IDLE";
						break;
					case SCHED_FIFO:
						schedstring = "FCFS";
						break;
					default:
						schedstring = "?";
				}

				snprintf(final + strlen(final),sizeof(final) - strlen(final),"%s%-*s %s",tabstart,formatvals[34],schedstring,tabbord);
			}
		}
		
		add_final(final,start->info.pid,start->info.state);

}

// bukvalno se ne secam koja je poenta ovog
void print_upper_menu_and_mod_offset(char ** fbuf, int fno, char ** dformats, int dformatno, char ** formats, int format_no, int * printno_export)
{

	GLOBAL_CURSE_OFFSET =  NOLOGO ? 1 : LOGO_HEIGHT;
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

//ponovo odstampaj vidljiv deo liste u terminalu, radi osvezavanja podataka u slucaju da je korisnik interakcijom ucinio neku promenu
void refreshList()
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

// ponovo odstampaj i listu procesa i header iznad
void displayScreen(char ** fbuf, int fno, char ** dformats, int dformatno, char ** formats, int format_no, int * printno_export)
{
	print_upper_menu_and_mod_offset(fbuf,fno,dformats,dformatno,formats,format_no,printno_export);
	refreshList();
}