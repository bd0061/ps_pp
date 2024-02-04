#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "structures.h"
#include "helper.h"

extern long btime;
extern long clock_ticks_ps;
extern int formatvals[23];


size_t iocheck(long long v,int type)
{
    int t = 0;
    double vd;
    
    if(v >= 1000000LL)
    {
        vd = (double)v / 1000000LL;
        t = 2;
    }
    else if (v >= 1000LL)
    {
        vd = (double)v / 1000LL;
        t = 1;
    }
    else 
    {
        vd = (double)v;
    }
    
    char cinfo[256];
    snprintf(cinfo,sizeof(cinfo),"%sB%s",t == 2 ? "M" : (t == 1 ? "K" : ""),type == 0 ? "" : "/s");

    return snprintf(NULL, 0, "%.1lf %s", vd, cinfo);
}

size_t dno(long long a)
{
    size_t i = 0;
    do { i++; } while( a/=10LL );
    
    return i;
}

size_t dno_u(unsigned long long a)
{
    size_t i = 0;
    do { i++; } while( a/=10ULL );
    
    return i;
}

void chomp(char * s)
{
    for(size_t i = 0; i < strlen(s); i++)
    {
        if(s[i] == '\n')
        {
            s[i] = '\0';
            return;
        }
    }
}

int in(int * arr , int a, int l)
{
    for(int i = 0; i < l; i++)
    {
        if(arr[i] == a)
            return 1;
    }
    return 0;
}


void addElement(PROCESS_LL** head, PROCESSINFO newInfo, PROCESSINFO_IO newIOInfo, CPUINFO newCPUinfo) {
    //ako proces vec postoji, samo mu updejtujemo podatke
    PROCESS_LL* trav = *head;

    while (trav != NULL) {
        
        if(trav->info.pid == newInfo.pid)
        {
            trav->info = newInfo;//napomena: u newInfo bufferu jedina io informacija je read(write)_t
            
            if(!trav->ioinfo.io_blocked)
            {
                trav->ioinfo.read_bytes_prev = trav->ioinfo.read_bytes_cur;
                trav->ioinfo.read_bytes_cur = newIOInfo.read_bytes_cur;

                trav->ioinfo.write_bytes_prev = trav->ioinfo.write_bytes_cur;
                trav->ioinfo.write_bytes_cur = newIOInfo.write_bytes_cur;

                trav->ioinfo.cancelled_write_bytes = newIOInfo.cancelled_write_bytes;
                FORMATCHECK_IO(trav->ioinfo.read_bytes_cur - trav->ioinfo.read_bytes_prev,formatvals[21],1);
                FORMATCHECK_IO(trav->ioinfo.write_bytes_cur - trav->ioinfo.write_bytes_prev,formatvals[22],1);
            }

            trav->cpuinfo.utime_prev = trav->cpuinfo.utime_cur;
            trav->cpuinfo.stime_prev = trav->cpuinfo.stime_cur;

            trav->cpuinfo.utime_cur = newCPUinfo.utime_cur;
            trav->cpuinfo.stime_cur = newCPUinfo.stime_cur;  

            return;
        }
        trav = trav->next;
    }



    //proces ne postoji, ubacujemo ga
    PROCESS_LL* newNode = (PROCESS_LL*)malloc(sizeof(PROCESS_LL));
    if (newNode == NULL) {
        fprintf(stderr, "linked list: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    newNode->info = newInfo; 
    
    newNode->cpuinfo.utime_cur = newCPUinfo.utime_cur;
    newNode->cpuinfo.utime_prev = newCPUinfo.utime_cur;
    newNode->cpuinfo.stime_cur = newCPUinfo.stime_cur;
    newNode->cpuinfo.stime_prev = newCPUinfo.stime_cur;

    if(!newIOInfo.io_blocked)
    {
        newNode->ioinfo.write_bytes_cur = newIOInfo.write_bytes_cur;
        newNode->ioinfo.write_bytes_prev = newIOInfo.write_bytes_cur;
        newNode->ioinfo.read_bytes_cur = newIOInfo.read_bytes_cur;
        newNode->ioinfo.read_bytes_prev = newIOInfo.read_bytes_cur;

        newNode->ioinfo.cancelled_write_bytes = newIOInfo.cancelled_write_bytes;
    }
    newNode->ioinfo.io_blocked = newIOInfo.io_blocked;

    
    newNode->next = NULL;

    if (*head == NULL) {
        *head = newNode;
    } else {
        PROCESS_LL* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

// Function to remove an element with a specific pid from the end of the linked list
void removeElement(PROCESS_LL** head, int pidToRemove) {
    if (*head == NULL) {
        return;
    }

    PROCESS_LL* current = *head;
    PROCESS_LL* previous = NULL;

    // Traverse the list to find the element with the specified pid
    while (current->next != NULL && current->info.pid != pidToRemove) {
        previous = current;
        current = current->next;
    }

    // Check if the element with the specified pid was found
    if (current->info.pid == pidToRemove) {
        if (previous == NULL) {
            // If the element to be removed is the first element
            *head = current->next;
        } else {
            previous->next = current->next;
        }

        // Free the memory of the removed node
        free(current);
    } else {
        fprintf(stderr, "linked list: element with pid %d not found\n", pidToRemove);
    }
}

void printList(PROCESS_LL* head) {
    PROCESS_LL* start = head;
    for (;start != NULL; start = start->next) {
        /*if(start->info.pid == 7073)
        {
            if(start->ioinfo.io_blocked)
            {
                //printf("[%d] - BLOCKED, SKIPPING...\n",start->info.pid);
                //c--;
                continue;
            }
            else 
            {
               printf("[%d] - byteread: %10lld B/s (total: %10lld)\tbyewrite: %10lld B/s (total: %10lld)\n",
                    start->info.pid,start->ioinfo.read_bytes_cur - start->ioinfo.read_bytes_prev, start->ioinfo.read_bytes_cur,
                    start->ioinfo.write_bytes_cur - start->ioinfo.write_bytes_prev, start->ioinfo.write_bytes_cur);

            }
        }*/

        //x:1 = y:100
        /*if(start->info.pid == 24223)
        {
        unsigned long cur = start->cpuinfo.utime_cur + start->cpuinfo.stime_cur;
        unsigned long prev = start->cpuinfo.utime_prev + start->cpuinfo.stime_prev;
        printf("[%d] %s %.2lf CPU%\n",start->info.pid, start->info.name, ((double)(cur-prev) / clock_ticks_ps) * 100);
        }*/

    }
}

// Function to free the memory allocated for the linked list
void freeList(PROCESS_LL* head) {
    PROCESS_LL* current = head;
    PROCESS_LL* next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

int compareByRes(const void *a, const void *b) {
    const PROCESS_LL *nodeA = *(const PROCESS_LL **)a;
    const PROCESS_LL *nodeB = *(const PROCESS_LL **)b;
    return (nodeB->info.res - nodeA->info.res);
}

int comparePID(const void *a, const void *b) {
    const PROCESS_LL *nodeA = *(const PROCESS_LL **)a;
    const PROCESS_LL *nodeB = *(const PROCESS_LL **)b;
    return (nodeA->info.pid - nodeB->info.pid);
}



void sortmem(PROCESS_LL **head) {
     if (*head == NULL) {
        return;
    }
    size_t count = 0;
    PROCESS_LL *temp = *head;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    PROCESS_LL **tempArray = (PROCESS_LL **)malloc(count * sizeof(PROCESS_LL *));
    if (tempArray == NULL) {
        return;
    }

    temp = *head;
    for (size_t i = 0; i < count; i++) {
        tempArray[i] = temp;
        temp = temp->next;
    }

    qsort(tempArray, count, sizeof(PROCESS_LL *), compareByRes);

    *head = tempArray[0];
    temp = *head;
    for (size_t i = 1; i < count; i++) {
        temp->next = tempArray[i];
        temp = temp->next;
    }
    temp->next = NULL;

    free(tempArray);
}

int compareCPU(const void *a, const void *b) {
    const PROCESS_LL *nodeA = *(const PROCESS_LL **)a;
    const PROCESS_LL *nodeB = *(const PROCESS_LL **)b;

    unsigned long curA = nodeA->cpuinfo.utime_cur + nodeA->cpuinfo.stime_cur;
    unsigned long curB = nodeB->cpuinfo.utime_cur + nodeB->cpuinfo.stime_cur;

    unsigned long prevA = nodeA->cpuinfo.utime_prev + nodeA->cpuinfo.stime_prev;
    unsigned long prevB = nodeB->cpuinfo.utime_prev + nodeB->cpuinfo.stime_prev;

    double formulaA = ((double)(curA - prevA) / clock_ticks_ps) * 100;
    double formulaB = ((double)(curB - prevB) / clock_ticks_ps) * 100;

    return (int)(formulaB - formulaA);
}

void sortCPU(PROCESS_LL **head) {
    if (*head == NULL) {
        return;
    }
    size_t count = 0;
    PROCESS_LL *temp = *head;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }

    PROCESS_LL **tempArray = (PROCESS_LL **)malloc(count * sizeof(PROCESS_LL *));
    if (tempArray == NULL) {
        return;
    }

    temp = *head;
    for (size_t i = 0; i < count; i++) {
        tempArray[i] = temp;
        temp = temp->next;
    }

    qsort(tempArray, count, sizeof(PROCESS_LL *), compareCPU);
    *head = tempArray[0];
    temp = *head;
    for (size_t i = 1; i < count; i++) {
        temp->next = tempArray[i];
        temp = temp->next;
    }
    temp->next = NULL;
    free(tempArray);
}

void sortPID(PROCESS_LL **head)
{
    if (*head == NULL) {
        return;
    }
    size_t count = 0;
    PROCESS_LL *temp = *head;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    PROCESS_LL **tempArray = (PROCESS_LL **)malloc(count * sizeof(PROCESS_LL *));
    if (tempArray == NULL) {
        return;
    }

    temp = *head;
    for (size_t i = 0; i < count; i++) {
        tempArray[i] = temp;
        temp = temp->next;
    }

    qsort(tempArray, count, sizeof(PROCESS_LL *), comparePID);

    *head = tempArray[0];
    temp = *head;
    for (size_t i = 1; i < count; i++) {
        temp->next = tempArray[i];
        temp = temp->next;
    }
    temp->next = NULL;

    free(tempArray);


}