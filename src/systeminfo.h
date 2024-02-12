#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

void getmeminfo(long long * memtotal, long long * memfree, long long * memavailable, long long * swaptotal, long long *swapfree);
void get_mount_point();
void getuptime();
double readSystemCPUTime();
void getcurrenttime(struct tm * s);
void getbtime();
long long getTimeInMilliseconds();


#endif