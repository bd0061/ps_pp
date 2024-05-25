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


#include "../../backend_api.h"
#include "components.h"
#include "dynamic_array_manager.h"


static void updateListInternalEx(int * pid_args, int pno, char ** ubuffer, int uno, char ** nbuffer, int nno,int fno, char ** fbuffer, char ** default_formats, int default_format_no, char ** formats, int format_no,
unsigned int flags)
{
	for(int i = 0; i < format_no; i++)
	{
		formatvals[i] = strlen(formats[i]);
	}
	formatvals[13] = 7; //mem% 
	formatvals[14] = 6; //cpu% 
	formatvals[15] = 5; //start
	clear_and_reset_array(&fps, &fps_size);
	///
	//updateListInternal(pid_args, pno, ubuffer, uno, nbuffer, nno,fno, fbuffer, default_formats, default_format_no, formats, format_no, flags);
	findprocs(&head, pid_args, pno, ubuffer, uno,nbuffer,nno,fbuffer,fno);
	sort(&head,flags);

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
	///
}


void start_main_ncurses()
{
	NAME_FILTER[0] = '\0';
	FILTERING = 0;
	int printno_export;
	int pid_args[500];
	int use_custom = 0;
	
	int current = REFRESH_RATE;
	int jFormat = 0;
	int ioFormat = 0;
	
    fps = NULL;
    fps_size = 0;
	selectedLine = 0;

	for(int i = 0; i < format_no; i++)
	{
		formatvals[i] = strlen(formats[i]);
	}



	GLOBAL_CURSE_OFFSET =  NOLOGO ? 1 : LOGO_HEIGHT;
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


	int clscFormat;
	int customFormat;
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

		sort(&head,flags);
		long long afterCritical = getTimeInMilliseconds();


		if(first_pspp || afterCritical - beforeCritical >= REFRESH_RATE)
	    {
	    	first_pspp = 0;
	    	beforeCritical = afterCritical;
	    	////
	    	countDown--;
			getuptime();
			getmeminfo(&memtotal, &memfree, &memavailable,&swaptotal,&swapfree);
			cpu_percent = readSystemCPUTime();	

			updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats,format_no,flags);
			////

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
	            if (getmouse(&event) == OK) 
	            {
	                if (event.bstate & BUTTON4_PRESSED) //scroll up
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
	                else if (event.bstate & BUTTON5_PRESSED) //scroll down
	                {
	                	if(selectedLine < fps_size - 1)
						{
							selectedLine++;
							if(selectedLine == y + start_pspp - GLOBAL_CURSE_OFFSET)
								start_pspp++;
							refreshList();
						}
	                  
	                }
	                else if (event.bstate & BUTTON1_PRESSED && event.y >= LIST_START /*&& event.x <= printno_export*/) {
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

					updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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
			else if(ch == KILLKILL_KEY && fps_size > 0 && !HELP_MODE)
			{
				if(kill(fps[selectedLine].pid,SIGKILL) == 0)
				{
					snprintf(INFOMSG,sizeof(INFOMSG),"\tKilled [%d] (SIGKILL)\t",fps[selectedLine].pid);
					SUCCESS = 1;
					countDown = 5;

					updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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
					snprintf(INFOMSG,sizeof(INFOMSG),"\tCouldn't kill [%d] (SIGKILL): %s\t",fps[selectedLine].pid,strerror(errno));
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

				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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

				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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

				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

			}
			else if(ch == MEMSORT_KEY && fps_size > 0 && flags != SORT_MEM && !HELP_MODE)
			{
				flags = SORT_MEM;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by mem\t");
				SUCCESS = 1;
				countDown = 5;
				sort(&head,flags);
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

			}
			else if(ch == TIMESORT_KEY && fps_size > 0 && flags != SORT_TIME && !HELP_MODE)
			{
				flags = SORT_TIME;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by start time (newest first)\t");
				SUCCESS = 1;
				countDown = 5;
				sort(&head,flags);
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

			}
			else if(ch == TIMESORT_KEY_OLDEST && fps_size > 0 && flags != SORT_TIME_OLDEST && !HELP_MODE)
			{
				flags = SORT_TIME_OLDEST;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by start time (oldest first)\t");
				SUCCESS = 1;
				countDown = 5;
				sort(&head,flags);
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

			}
			else if(ch == CPUSORT_KEY && fps_size > 0 && flags != SORT_CPU && !HELP_MODE)
			{
				flags = SORT_CPU;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by CPU%%\t");
				SUCCESS = 1;
				countDown = 5;
				sort(&head,flags);
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
			}
			else if(ch == NORMALSORT_KEY && fps_size > 0 && flags != SORT_PID && !HELP_MODE)
			{
				flags = SORT_PID;
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by default(PID)\t");
				SUCCESS = 1;
				countDown = 5;
				sort(&head,SORT_PID);
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);

			}
			else if(ch == PRIOSORT_KEY && fps_size > 0 && flags != SORT_PRIO && !HELP_MODE)
			{
				flags = SORT_PRIO;
				
				snprintf(INFOMSG,sizeof(INFOMSG),"\tNow sorting by priority\t");
				SUCCESS = 1;
				countDown = 5;
				
				sort(&head,flags);
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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
					
					updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
					displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
				}
			}
			else if(ch == BACK_KEY && fps_size > 0 && selectedLine > 0 && !HELP_MODE)
			{
				start_pspp = 0;
				selectedLine = 0;
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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
						updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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
					updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
				displayScreen(f.buffer, f.no, default_formats, default_format_no, formats, format_no, &printno_export);
			}
			else if(ch == KEY_BACKSPACE && strlen(NAME_FILTER) > 0)
			{
				NAME_FILTER[strlen(NAME_FILTER) - 1] = '\0';
				clear();
				print_art();
				selectedLine = 0;
				start_pspp = 0;
				updateListInternalEx(pid_args,p.no,u.buffer,u.no,n.buffer,n.no,f.no,f.buffer,default_formats,default_format_no,formats, format_no,flags);
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


}