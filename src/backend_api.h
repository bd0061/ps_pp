#ifndef BACKEND_API_H
#define BACKEND_API_H


#include "infocollect.h"
#include "systeminfo.h"
#include "structures.h"
#include "helper.h"
#include "parseopts.h"
#include "configreader.h"

extern struct option f;
extern struct option p;
extern struct option n;
extern struct option u;


extern PROCESS_LL * head;
extern PROCESS_LL * start;
extern char MOUNT_POINT[256];
extern unsigned int flags;
extern unsigned long long allprocs;
extern unsigned long long countprocs;
extern long clock_ticks_ps;
extern long pgsz;
extern long btime;
extern unsigned long uptime;

extern char *formats[35];
extern const int format_no;
extern char *default_formats[35];
extern int default_format_no;
extern long long memtotal;
extern long long memfree; 
extern long long memavailable;
extern long long swaptotal;
extern long long swapfree;
extern double cpu_percent;

void updateListInternal
(int * pid_args, int pno, char ** ubuffer, int uno, char ** nbuffer, int nno,int fno, char ** fbuffer, char ** default_formats, int default_format_no, char ** formats, int format_no,
unsigned int flags);


#endif