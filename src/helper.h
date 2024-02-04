#ifndef HELPER_H
#define HELPER_H
#include<stddef.h>

size_t dno(long long a);
size_t dno_u(unsigned long long a);

#define FORMATCHECK_NUM(X, Y) if (dno((X)) > (Y)) (Y) = dno((X))
#define FORMATCHECK_UNSIGNED_NUM(X, Y) if (dno_u((X)) > (Y)) (Y) = dno_u((X))

#define FORMATCHECK_STRING(X, Y) if (strlen((X)) > (Y)) (Y) = strlen(X)

#define FORMATCHECK_IO(X,Y,Z) if(iocheck((X),Z) > (Y)) (Y) = iocheck(X,Z)

size_t iocheck(long long v, int type);



void chomp(char * s);
int in(int * arr , int a, int l);
void addElement(PROCESS_LL** head, PROCESSINFO newInfo, PROCESSINFO_IO newIOInfo, CPUINFO newCPUinfo);
void removeElement(PROCESS_LL** head, int pidToRemove);
void printList(PROCESS_LL* head);
void freeList(PROCESS_LL* head);

int compareByRes(const void *a, const void *b);
void sortmem(PROCESS_LL **head);

void sortCPU(PROCESS_LL **head);
int compareCPU(const void *a, const void *b);

void sortPID(PROCESS_LL **head);
int comparePID(const void *a, const void *b);



#endif