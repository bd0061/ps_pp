#ifndef INFOCOLLECT_H
#define INFOCOLLECT_H

void getmeminfo(long long * memtotal, long long * memfree, long long * memavailable, long long * swaptotal, long long *swapfree);

void findprocs(PROCESS_LL ** head, int * pid_args, int pid_count, char ** user_args, int user_count, char ** name_args, int name_count);


#endif