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

#include "structures.h"
#include "infocollect.h"
#include "helper.h"
#include "parseopts.h"
#include "configreader.h"
#include "systeminfo.h"


#include "frontend/curses/dynamic_array_manager.h"
#include "frontend/curses/components.h"


PROCESS_LL * head;
PROCESS_LL * start;

unsigned long long allprocs;
unsigned long long countprocs;


char MOUNT_POINT[256];

//NA OSNOVU OVE PROMENLJIVE ODREDITI PO CEMU SORTIRATI
unsigned int flags = SORT_PID;
double cpu_percent;

get_mount_point();

//SVE PODRZANE OPCIJE ZA "FORMAT"
char *formats[] = 		  
{"PID","NAME","STATE","PPID","TTY","UTIME","STIME","PRIO","NICE","THREADNO",
"VIRT","RES","OWNER", "MEM%", "CPU%","START","SID","PGRP","C_WRITE","IO_READ","IO_WRITE","IO_READ/s","IO_WRITE/s","SHR",
"MINFLT","CMINFLT","MAJFLT","CMAJFLT","CUTIME","CSTIME","RESLIM","EXITSIG","PROCNO","RTPRIO","POLICY"};
const int format_no = 35;
/////////////////////////////////////////////////////////////////////////////////////




//OPCIJE KOJE CE SE POKAZATI UKOLIKO IH KORISNIK NE EKSPLICITNO SPECIFIRA
char *default_formats[35] = 
{"PID","STATE","PPID","TTY","PRIO","NICE", "SHR",
"VIRT","RES","OWNER", "MEM%", "CPU%","START","NAME"};
int default_format_no = 14;
/////////////////////////////////////////////////////////////////////////////////////




static void updateSysinfo()
{
	getuptime();
	getmeminfo(&memtotal, &memfree, &memavailable,&swaptotal,&swapfree);
	cpu_percent = readSystemCPUTime();	
}


/*ponovo pokupi podatke u spregnutu listu, obrisi mrtve procese i procese koji nam vise ne trebaju za trenutnu selekciju. */
void updateListInternal
(int * pid_args, int pno, char ** ubuffer, int uno, char ** nbuffer, int nno,int fno, char ** fbuffer, char ** default_formats, int default_format_no, char ** formats, int format_no,
unsigned int flags)
{
	
	findprocs(&head, pid_args, p.no, u.buffer, u.no,n.buffer,n.no,f.buffer,f.no);
	sort(&head,flags);
	for(int i = 0; i < format_no; i++)
	{
		formatvals[i] = strlen(formats[i]);
	}
	formatvals[13] = 7; //mem% 
	formatvals[14] = 6; //cpu% 
	formatvals[15] = 5; //start

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

int main(int argc, char ** argv)
{
	//UNOS KRATKIH I DUGIH PODRZANIH OPCIJA
	/* STRUCT OPTION:
	- KRATAK NAZIV
	- PUN NAZIV
	- BAFER GDE CE BITI VREDNOSTI ARGUMENATA OPCIJE (STRINGOVI)
	- BROJ ARGUMENATA
	*/
	struct option f = {"-f","--format",NULL,0};
	struct option p = {"-p","--pid",NULL,0};
	struct option u = {"-u","--user",NULL,0};
	struct option n = {"-n","--name",NULL,0};
	/////////////////////////////////////////////////////////////////////////////////////
	

	//PROVERI DA LI JE UNOS SA KOMANDNE LINIJE LOGICAN (NE DOZVOLITI NEPOSTOJECE OPCIJE)
	struct arg_parse a = {{&f,&p,&u,&n},4};
	parseopts(argv,&a);
	/////////////////////////////////////////////////////////////////////////////////////

	
	//NAKON UNOSA, PROVERITI DA LI SU PODACI SEMANTICKI SMISLENI (DA LI UNESENI FORMATI PRIPADAJU LISTI FORMATA, DA LI SPECIFIRANI KORISNICI POSTOJE I SL)
	sanitycheck(f.buffer,f.no, formats, format_no,
				p.buffer,p.no,
				u.buffer,u.no);
	/////////////////////////////////////////////////////////////////////////////////////
	



	start_main_ncurses_loop();
}