#ifndef STRUCTURES_H
#define STRUCTURES_H
#include<time.h>
// /proc/pid/stat truncates name to 15 char

#define CURRENT_UNIX_TIME (time(NULL))

typedef struct PROCESSINFO 
{
	int pid;
	char name[4096];
	char state;
	int ppid;
	int pgrp;
	int tty;
	int sid;
	
	unsigned long utime;
	unsigned long stime;
	
	long prio;
	long nice;
	long threadno;
	unsigned long virt;
	long res; //pages
	char user[128];
	char ttyname[128];
	unsigned long long starttime; //ticks after boot time
	struct tm start_struct;
	long starttime_secs;



	unsigned long utime_prev;
	unsigned long stime_prev;



} PROCESSINFO;

typedef struct PROCESSINFO_IO
{
	long long read_bytes_prev;
	long long write_bytes_prev;
	long long read_bytes_cur;
	long long write_bytes_cur;
	
	long long cancelled_write_bytes;
	int io_blocked;
} PROCESSINFO_IO;

typedef struct CPUINFO
{
	unsigned long utime_cur;
	unsigned long utime_prev;
	
	unsigned long stime_cur;
	unsigned long stime_prev;

} CPUINFO;


typedef struct PROCESS_LL
{
	PROCESSINFO info;
	PROCESSINFO_IO ioinfo;
	CPUINFO cpuinfo;
	struct PROCESS_LL * next;

} PROCESS_LL;


#endif