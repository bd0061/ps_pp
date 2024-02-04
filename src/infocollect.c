#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include "structures.h"
#include "helper.h"
#include "regex_module.h"


extern int formatvals[23];
extern long clock_ticks_ps;
extern long btime;
extern unsigned long long cputicks;
extern unsigned long long allprocs;
extern unsigned long long countprocs;
extern long pgsz;


int collect_io_info(PROCESSINFO_IO * buffer, char * pidname)
{
    char iopath[1024];
    int line = 1;

    snprintf(iopath,sizeof(iopath),"/proc/%s/io", pidname);
    FILE * io = fopen(iopath,"r");
    if(io == NULL)
    {
        if(errno == ENOENT)
        {
            return -1;
        }
        if(errno == EACCES)
        {
            return -2;
        }
        perror("collect_io_info: fopen");
        exit(EXIT_FAILURE);
    }

    char b[1024];
    while(fgets(b,sizeof(b),io) != NULL)
    {
        switch(line)
        {
            case 5:
                sscanf(b,"read_bytes: %lld", &(buffer->read_bytes_cur));
                break;
            case 6:
                sscanf(b,"write_bytes: %lld", &(buffer->write_bytes_cur));
                break;
            case 7:
                sscanf(b,"cancelled_write_bytes: %lld", &(buffer->cancelled_write_bytes));
                goto out;
        }
        
        line++;
    }
    out:
    fclose(io);
    if(line != 7) // ?? ne bi trebalo nikad da se desi ali se ipak nekako desi za jedan specifican systemd proces(iz nekog razloga)
    {
        //neke procese iz nekog razloga mozemo otvoriti a ne mozemo ih citati(ili se barem u njima ne nalazi
        //ono sto nam treba i u tom slucaju vracamo identicno kao i za eacces
        return -2; // ???

    }

    return 0;

}


void handleformat(PROCESSINFO t)
{
    FORMATCHECK_NUM(t.pid,formatvals[0]);
    FORMATCHECK_STRING(t.name,formatvals[1]);
    FORMATCHECK_NUM(t.ppid,formatvals[3]);
    FORMATCHECK_STRING(t.ttyname,formatvals[4]);
    
    FORMATCHECK_UNSIGNED_NUM(t.utime,formatvals[5]);
    FORMATCHECK_UNSIGNED_NUM(t.stime,formatvals[6]);
    
    FORMATCHECK_NUM(t.prio,formatvals[7]);
    FORMATCHECK_NUM(t.nice,formatvals[8]);
    FORMATCHECK_NUM(t.threadno,formatvals[9]);
    
    FORMATCHECK_UNSIGNED_NUM((t.virt)/1024UL,formatvals[10]); //kilobajti
    
    FORMATCHECK_NUM(t.res * (pgsz/1024L),formatvals[11]); // iz stranica u kilobajte
    FORMATCHECK_STRING(t.user,formatvals[12]);
    
    /* ne treba proveravati za state(jedan karaker uvek)
     * kao i za cpu% i mem%(ograniceni procentno na max 4 znaka 0,XY)
     * takodje start jer je njegova sama duzina(5) jednaka ili veca od svih mogucih varijanti(XY:ZQ, MONXY,XYWZ)
    */

    FORMATCHECK_NUM(t.sid,formatvals[16]);
    FORMATCHECK_NUM(t.pgrp,formatvals[17]);
    


}


void getmeminfo(long long * memtotal, long long * memfree, long long * memavailable, long long * swaptotal, long long *swapfree)
{
    FILE * fp = fopen("/proc/meminfo","r");
    if(fp == NULL)
    {
        perror("getmeminfo: fopen");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    int c = 1;
    while ( fgets(buffer, sizeof(buffer), fp) != NULL )
    {
        if(c == 1)
            sscanf(buffer, "MemTotal: %lld", memtotal);
        else if (c == 2)
            sscanf(buffer, "MemFree: %lld", memfree);
        else if (c == 3)
        {
            sscanf(buffer, "MemAvailable: %lld", memavailable);
        }
        else if (c == 15)
        {
            sscanf(buffer, "SwapTotal: %lld", swaptotal);

        }
        else if (c == 16)
        {
           sscanf(buffer, "SwapFree: %lld", swapfree);
            break;
        }

        c++;

    }
    if(c != 16)
    {
        fprintf(stderr,"unknown error reading meminfo");
        exit(EXIT_FAILURE);
    }
    fclose(fp);

}

/*skladisti informacije o procesima na osnovu datih opcija i filtera
 *
 *  !!! GLAVNI PROBLEM: KONKURENTAN PRISTUP PROCFS IZMEDJU NAS I KERNELA !!!
 * 
 * JEDNO OD RESENJA: provera da li direktorijum i dalje postoji(samo kad je apsolutno neophodno) 
 * npr kod citanja dodatnih informacija, ako jeste ne mozemo vise informacija da pokupimo jer je proces upravo prekinut, 
 * samo obustaviti trenutnu iteraciju i preci dalje 
 */
void findprocs(PROCESS_LL ** head, int * pid_args, int pid_count, char ** user_args, int user_count, char ** name_args, int name_count) {
    
    allprocs = 0;
    countprocs = 0;
    const char *PROC = "/proc"; 
    DIR *dir = opendir(PROC);
    if (dir == NULL) {
        perror("findprocs: fopen:");
        exit(EXIT_FAILURE);
    }
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if(entry->d_type == DT_DIR && regexec(&regex, entry->d_name, 0, NULL, 0) == 0)
        {
            allprocs++;

            if (pid_count != 0)
            {
                if(!in(pid_args, atoi(entry->d_name), pid_count))
                {
                    continue;
                }
            }
            
            PROCESSINFO t;
            
            char fullpath[300];
            snprintf(fullpath,sizeof(fullpath),"/proc/%s/stat", entry->d_name);
            FILE * infofile = fopen(fullpath,"r");

            //ekstremno redak slucaj: proces je ubijen onog momenta kada pokusamo da citamo njegove fajlove
            //zato gledamo da errno bude samo iz sistemskih razloga(memorijske greske itd)
            if(infofile == NULL) 
            {
                if(errno == ENOENT) //process prekinut upravo sad
                {
                    continue;
                }
                perror("findprocs: error opening stat file");
                exit(EXIT_FAILURE);
            }
            char buffer[4096];
            

            if (fgets(buffer, sizeof(buffer), infofile) == NULL)
            {
                fprintf(stderr,"error reading stat file\n");
                fclose(infofile);
                exit(EXIT_FAILURE);
            }
            sscanf(
            buffer,
            "%d %*s %c %d "
            //////////////////////////////////////////////
            "%d %d "
            //////////////////////////////////////////////
            "%d "
            //////////////////////////////////////////////
            "%*s %*s %*s %*s %*s %*s "
            //////////////////////////////////////////////
            "%lu %lu "
            //////////////////////////////////////////////
            "%*s %*s"
            //////////////////////////////////////////////
            "%ld %ld %ld "
            //////////////////////////////////////////////
            "%*s %llu"
            //////////////////////////////////////////////
            "%lu %ld",
            &(t.pid), &(t.state), &(t.ppid), &(t.sid), &(t.pgrp),
            &(t.tty),
            &(t.utime), &(t.stime),
            &(t.prio), &(t.nice), &(t.threadno),
            &(t.starttime),
            &(t.virt),&(t.res)
            );



            snprintf(fullpath,sizeof(fullpath),"/proc/%s/cmdline", entry->d_name);
            FILE * fullcommand = fopen(fullpath,"r");
            if(fullcommand == NULL)
            {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
           
            if (fgets(buffer, sizeof(buffer), fullcommand) == NULL) //kernel thread
            {
                fclose(fullcommand);
                snprintf(fullpath,sizeof(fullpath),"/proc/%s/comm", entry->d_name);
                fullcommand = fopen(fullpath, "r");
                if(fullcommand == NULL || fgets(buffer, sizeof(buffer), fullcommand) == NULL)
                {
                    fprintf(stderr, "commm fallback error");
                    exit(EXIT_FAILURE);
                }
            }

            chomp(buffer);
            strncpy(t.name,buffer,sizeof(buffer)-1);
            t.name[sizeof(t.name) - 1] = '\0';
            if(name_count != 0)
            {
                int ok = 0;
                for(int i = 0; i < name_count; i++)
                {
                    if(strstr(t.name,name_args[i]) != NULL)
                    {
                        ok = 1;
                        break;
                    }
                }
                if(!ok)
                {
                    fclose(infofile);
                    fclose(fullcommand);
                    continue;
                }
            }
            //begin
                char _statuspath[1024];
                snprintf(_statuspath,sizeof(_statuspath),"/proc/%s/status", entry->d_name);
                FILE * statusfile = fopen(_statuspath,"r");
                if(statusfile == NULL)
                {
                    if(errno == ENOENT) //proces prekinut upravo sad
                    {
                        fclose(infofile);
                        fclose(fullcommand);
                        continue;
                    }
                    perror("Error opening status file");
                    exit(EXIT_FAILURE);
                }
                int line = 1;
                char bfr[1024];
                int _uid;
                while ( fgets(bfr, sizeof(bfr), statusfile) != NULL )
                {
                    if(line == 9)
                    {
                        sscanf(bfr, "Uid: %d", &_uid);
                        break;
                    }
                    line++;
                }
                if(line != 9)
                {
                    fprintf(stderr,"status read error\n");
                    exit(EXIT_FAILURE);
                }
                // ... //
                struct passwd *pw = getpwuid(_uid);
                struct passwd pwcopy = *pw;
                if(pw == NULL)
                {
                    fprintf(stderr, "infocollect: unknown uid processing error\n");
                    exit(EXIT_FAILURE);
                }
                strcpy(t.user,pwcopy.pw_name);
            //end

            int ok_usr = 0;
            if(user_count != 0) //korisnik zeli da filtrira po korisnicima
            {
                for(int i = 0; i < user_count; i++)
                {
                    if(strcmp(t.user,user_args[i]) == 0)
                    {
                        ok_usr = 1;
                        break;
                    }
                }
                if(!ok_usr)
                {
                    fclose(infofile);
                    fclose(fullcommand);
                    fclose(statusfile);
                    continue;
                }


            }
            char linkpath[1024];
            char tname[256];
            struct stat file_stat;

            snprintf(linkpath,sizeof(linkpath),"/proc/%s/fd/0", entry->d_name);

            if (stat(linkpath, &file_stat) == -1) 
            {
                if(errno == EACCES)
                {
                    tname[0] = '?';
                    tname[1] = '\0';
                    strcpy(t.ttyname,tname);
                    goto done;
                }
                else if(errno == ENOENT)
                    continue; //problem konkurentnosti, abortiraj
                else
                {
                    perror("findprocs: stat");
                    exit(EXIT_FAILURE);
                }    

            }


            ssize_t len;
            if((len = readlink(linkpath, tname, sizeof(tname) - 1)) == -1)
            {
                if(errno == ENOENT) 
                {
                    fclose(infofile);
                    fclose(fullcommand);
                    fclose(statusfile);
                    continue; //process dead mid read
                }
                perror("findprocs: readlink");
            }
            
            tname[len] = '\0';

            if(strcmp(tname,"/dev/null") == 0 || strstr(tname,"/dev/") == NULL)
            {
                strncpy(tname,"?",sizeof(tname) - 1);
                tname[sizeof(tname) - 1] = '\0';
                strcpy(t.ttyname,tname);
            }
            else
            {
                strcpy(t.ttyname,tname + 5);
            }
            

            done: ;

            long procsecs = (t.starttime)/clock_ticks_ps;

            t.starttime_secs = procsecs;
            
            long procstarttime = btime + procsecs;
            localtime_r(&procstarttime, &(t.start_struct));
            
            PROCESSINFO_IO tio;
            int iostatus = collect_io_info(&tio,entry->d_name);

            if(iostatus == -1)
            {

                fclose(infofile);
                fclose(fullcommand);
                fclose(statusfile);
                continue;
            }
            else if(iostatus == -2)
            {
                //negativna vrednost kao indikacija da ne mozemo da pristupimo io informacijama datog procesa

                //napomena: ovaj program pretpostavlja da proces za koji se ne mogu izvuci io podaci nece te podatke moci da ocita do kraja
                //zivotnog ciklusa procesa
                tio.io_blocked = 1;
            }
            else 
            {
                tio.io_blocked = 0;
            }
            
            countprocs++;
            CPUINFO tcpu;
            tcpu.utime_cur = t.utime;
            tcpu.stime_cur = t.stime;
            
            addElement(head,t,tio,tcpu);
            if(!tio.io_blocked)
            {
                FORMATCHECK_IO(tio.cancelled_write_bytes,formatvals[18],0);
                FORMATCHECK_IO(tio.read_bytes_cur,formatvals[19],0);
                FORMATCHECK_IO(tio.write_bytes_cur,formatvals[20],0);
            }
            handleformat(t);

            fclose(infofile);
            fclose(fullcommand);
            fclose(statusfile);
        }
    }

    closedir(dir);
}


