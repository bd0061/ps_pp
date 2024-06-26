#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>


int COLOR_BG[3];
int COLOR_TEXT[3];
int COLOR_PRCNT_BAR[3];
int COLOR_SELECTPROC[3];
int COLOR_INFOTEXT[3];
int COLOR_INFO_SUC[3];
int COLOR_INFO_ERR[3];

char QUIT_KEY;
char KILL_KEY;
char IO_KEY;
char ID_KEY;
char CLASSIC_KEY;
char MEMSORT_KEY;
char CPUSORT_KEY;
char NORMALSORT_KEY;
char CUSTOM_KEY;
char BACK_KEY;
char HELP_KEY;
char END_KEY;
char NICEPLUS_KEY;
char NICEMINUS_KEY;
char PRIOSORT_KEY;
char TOGGLESUSPEND_KEY;
char KILLKILL_KEY;
char TIMESORT_KEY;
char TIMESORT_KEY_OLDEST;
int NOLOGO;
#define PS_PP_COLORS 7
#define PS_PP_KEYS 19

static int isempty(char * s)
{
	for(int i = 0; s[i] != '\0'; i++)
	{
		if(s[i] != '\t' && s[i] != ' ' && s[i] != '\n')
			return 0;
	}
	return 1;
}

void readvals()
{
	NOLOGO = 0;

	int COLOR_BG_CHECK = 0;
	int COLOR_TEXT_CHECK = 0;
	int COLOR_PRCNT_BAR_CHECK = 0;
	int COLOR_SELECTPROC_CHECK = 0;
	int COLOR_INFOTEXT_CHECK = 0;
	int COLOR_INFO_SUC_CHECK = 0;
	int COLOR_INFO_ERR_CHECK = 0;

	int QUIT_KEY_CHECK 	= 0;
	int KILL_KEY_CHECK = 0;
	int IO_KEY_CHECK = 0;
	int ID_KEY_CHECK = 0;
	int CLASSIC_KEY_CHECK = 0;
	int MEMSORT_KEY_CHECK = 0;
	int CPUSORT_KEY_CHECK = 0;
	int NORMALSORT_KEY_CHECK = 0;
	int TIMESORT_KEY_CHECK = 0;
	int TIMESORT_KEY_OLDEST_CHECK = 0;
	int CUSTOM_KEY_CHECK = 0;
	int BACK_KEY_CHECK = 0;
	int HELP_KEY_CHECK = 0;
	int END_KEY_CHECK = 0;
	int NICEPLUS_KEY_CHECK = 0;
	int NICEMINUS_KEY_CHECK = 0;
	int PRIOSORT_KEY_CHECK = 0;
	int TOGGLESUSPEND_KEY_CHECK = 0;
	int KILLKILL_KEY_CHECK = 0;
	int defenv = 1;
	
	FILE * configreader = NULL;
	char fullpath[1024];
	char * usr = getenv("USER");
	if(usr == NULL)
		goto defaultcolor;

	snprintf(fullpath,sizeof(fullpath),"/home/%s/.config/ps_pp.conf.d/ps_pp.conf",usr);

	again: ;
	
	configreader = fopen(fullpath,"r");

	if(configreader == NULL)
	{
		if(errno == ENOENT)
		{
			if(strcmp(fullpath,"/etc/ps_pp.conf.d/ps_pp.conf") != 0)
			{
				snprintf(fullpath,sizeof(fullpath),"/etc/ps_pp.conf.d/ps_pp.conf");
				goto again;
			}
			goto defaultcolor;
		}
		perror("configreader: fopen");
		exit(EXIT_FAILURE);
	}

	char line[1024];
	int i = 0;
	int failed = 0;
	while(fgets(line,sizeof(line),configreader) != NULL)
	{
		i++;
		if(line[0] == '#') continue;
		
		if(sscanf(line,"COLOR_BG=(%d,%d,%d)\n",&COLOR_BG[0],&COLOR_BG[1],&COLOR_BG[2]))
		{
			COLOR_BG_CHECK = 1;
		}
		else if(sscanf(line,"COLOR_TEXT=(%d,%d,%d)\n",&COLOR_TEXT[0],&COLOR_TEXT[1],&COLOR_TEXT[2]))
		{
			COLOR_TEXT_CHECK = 1;
		}
		else if(sscanf(line,"COLOR_PRCNT_BAR=(%d,%d,%d)\n",&COLOR_PRCNT_BAR[0],&COLOR_PRCNT_BAR[1],&COLOR_PRCNT_BAR[2]))
		{
			COLOR_PRCNT_BAR_CHECK = 1;
		}
		else if(sscanf(line,"COLOR_SELECTPROC=(%d,%d,%d)\n",&COLOR_SELECTPROC[0],&COLOR_SELECTPROC[1],&COLOR_SELECTPROC[2]))
		{
			COLOR_SELECTPROC_CHECK = 1;
		}
		else if(sscanf(line,"COLOR_INFOTEXT=(%d,%d,%d)\n",&COLOR_INFOTEXT[0],&COLOR_INFOTEXT[1],&COLOR_INFOTEXT[2]))
		{
			COLOR_INFOTEXT_CHECK = 1;
		}
		else if(sscanf(line,"COLOR_INFO_SUC=(%d,%d,%d)\n",&COLOR_INFO_SUC[0],&COLOR_INFO_SUC[1],&COLOR_INFO_SUC[2]))
		{
			COLOR_INFO_SUC_CHECK = 1;
		}
		else if(sscanf(line,"COLOR_INFO_ERR=(%d,%d,%d)\n",&COLOR_INFO_ERR[0],&COLOR_INFO_ERR[1],&COLOR_INFO_ERR[2]))
		{
			COLOR_INFO_ERR_CHECK = 1;
		}
		else if(sscanf(line,"QUIT_KEY=%c",&QUIT_KEY))
		{
			QUIT_KEY_CHECK = 1;
		}
		else if(sscanf(line,"KILL_KEY=%c",&KILL_KEY))
		{
			KILL_KEY_CHECK = 1;
		}
		else if(sscanf(line,"IO_KEY=%c",&IO_KEY))
		{
			IO_KEY_CHECK = 1;
		}
		else if(sscanf(line,"ID_KEY=%c",&ID_KEY))
		{
			ID_KEY_CHECK = 1;
		}
		else if(sscanf(line,"CLASSIC_KEY=%c",&CLASSIC_KEY))
		{
			CLASSIC_KEY_CHECK = 1;
		}
		else if(sscanf(line,"MEMSORT_KEY=%c",&MEMSORT_KEY))
		{
			MEMSORT_KEY_CHECK = 1;
		}
		else if(sscanf(line,"CPUSORT_KEY=%c",&CPUSORT_KEY))
		{
			CPUSORT_KEY_CHECK = 1;
		}
		else if(sscanf(line,"NORMALSORT_KEY=%c",&NORMALSORT_KEY))
		{
			NORMALSORT_KEY_CHECK = 1;
		}
		else if(sscanf(line,"CUSTOM_KEY=%c",&CUSTOM_KEY))
		{
			CUSTOM_KEY_CHECK = 1;
		}
		else if(sscanf(line,"BACK_KEY=%c",&BACK_KEY))
		{
			BACK_KEY_CHECK = 1;
		}
		else if(sscanf(line,"HELP_KEY=%c",&HELP_KEY))
		{
			HELP_KEY_CHECK = 1;
		}
		else if(sscanf(line,"END_KEY=%c",&END_KEY))
		{
			END_KEY_CHECK = 1;
		}
		else if(sscanf(line,"NICEPLUS_KEY=%c",&NICEPLUS_KEY))
		{
			NICEPLUS_KEY_CHECK = 1;
		}
		else if(sscanf(line,"NICEMINUS_KEY=%c",&NICEMINUS_KEY))
		{
			NICEMINUS_KEY_CHECK = 1;
		}
		else if(sscanf(line,"PRIOSORT_KEY=%c",&PRIOSORT_KEY))
		{
			PRIOSORT_KEY_CHECK = 1;
		}
		else if(sscanf(line,"TIMESORT_KEY=%c",&TIMESORT_KEY))
		{
			TIMESORT_KEY_CHECK = 1;
		}
		else if(sscanf(line,"TIMESORT_KEY_OLDEST=%c",&TIMESORT_KEY_OLDEST))
		{
			TIMESORT_KEY_OLDEST_CHECK = 1;
		}
		else if(sscanf(line,"TOGGLESUSPEND_KEY=%c",&TOGGLESUSPEND_KEY))
		{
			TOGGLESUSPEND_KEY_CHECK = 1;
		}
		else if(sscanf(line,"KILLKILL_KEY=%c",&KILLKILL_KEY))
		{
			KILLKILL_KEY_CHECK = 1;
		}
		else if (strcmp(line,"NOLOGO\n") == 0)
		{
			NOLOGO = 1;
		}
		else if(!isempty(line))
		{
			fprintf(stderr,"%s:%d unknown configuration parameter: %s\n",fullpath, i, line);
			failed = 1;
		}
	}
	if(failed)
	{
		fclose(configreader);
		exit(EXIT_FAILURE);
	}

	defaultcolor:
	 if(!COLOR_BG_CHECK)
	 {
	 	COLOR_BG[0] = 0; COLOR_BG[1] = 0; COLOR_BG[2] = 90;
	 }
	 if(!COLOR_TEXT_CHECK)
	 {
	 	COLOR_TEXT[0] = 900; COLOR_TEXT[1] = 900; COLOR_TEXT[2] = 1000;
	 }
	 if(!COLOR_PRCNT_BAR_CHECK)
	 {
	 	COLOR_PRCNT_BAR[0] = 1000; COLOR_PRCNT_BAR[1] = 500; COLOR_PRCNT_BAR[2] = 500;
	 }
	 if(!COLOR_SELECTPROC_CHECK)
	 {
	 	COLOR_SELECTPROC[0] = 600; COLOR_SELECTPROC[1] = 600; COLOR_SELECTPROC[2] = 1000;
	 }
	 if(!COLOR_INFOTEXT_CHECK)
	 {
	 	COLOR_INFOTEXT[0] = 400; COLOR_INFOTEXT[1] = 400; COLOR_INFOTEXT[2] = 900;
	 }
	 if(!COLOR_INFO_SUC_CHECK)
	 {
	 	COLOR_INFO_SUC[0] = 100; COLOR_INFO_SUC[1] = 600; COLOR_INFO_SUC[2] = 100;
	 }
	 if(!COLOR_INFO_ERR_CHECK)
	 {
	 	COLOR_INFO_ERR[0] = 900; COLOR_INFO_ERR[1] = 200; COLOR_INFO_ERR[2] = 200;
	 }

	 if(!QUIT_KEY_CHECK)
	 	QUIT_KEY = 'q';
	 
	 if(!KILL_KEY_CHECK)
	 	KILL_KEY = 'k';
	 
	 if(!IO_KEY_CHECK)
	 	IO_KEY = 'i';
	
	 if(!ID_KEY_CHECK)
	 	ID_KEY = 'j';
	
	 if(!CLASSIC_KEY_CHECK)
	 	CLASSIC_KEY = 'c';
	
	 if(!MEMSORT_KEY_CHECK)
	 	MEMSORT_KEY = 'm';
	
	 if(!CPUSORT_KEY_CHECK)
	 	CPUSORT_KEY = 'p';
	 
	 if(!NORMALSORT_KEY_CHECK)
	 	NORMALSORT_KEY = 'n';
	 
	 if(!TIMESORT_KEY_CHECK)
	 	TIMESORT_KEY = 'b';
	 
	 if(!TIMESORT_KEY_OLDEST_CHECK)
	 	TIMESORT_KEY_OLDEST = 'B';
	 
	 if(!CUSTOM_KEY_CHECK)
	 	CUSTOM_KEY = 'x';
	 
	 if(!BACK_KEY_CHECK)
	 	BACK_KEY = 'a';
	 
	 if(!HELP_KEY_CHECK)
	 	HELP_KEY = 'h';

	 if(!END_KEY_CHECK)
	 	END_KEY = 'z';
	 
	 if(!NICEPLUS_KEY_CHECK)
	 	NICEPLUS_KEY = 'w';
	 
	 if(!NICEMINUS_KEY_CHECK)
	 	NICEMINUS_KEY = 'e';
	 
	 if(!PRIOSORT_KEY_CHECK)
	 	PRIOSORT_KEY = 'l';
	 
	 if(!TOGGLESUSPEND_KEY_CHECK)
	 	TOGGLESUSPEND_KEY = 'g';
	 if(!KILLKILL_KEY_CHECK)
	 	KILLKILL_KEY = 'K';

	int * a[PS_PP_COLORS] = {COLOR_BG,COLOR_TEXT,COLOR_PRCNT_BAR,COLOR_SELECTPROC,COLOR_INFOTEXT,COLOR_INFO_SUC,COLOR_INFO_ERR};

	char b[PS_PP_KEYS] = {QUIT_KEY,KILL_KEY,IO_KEY,ID_KEY,CLASSIC_KEY,MEMSORT_KEY,CPUSORT_KEY,NORMALSORT_KEY,PRIOSORT_KEY,TIMESORT_KEY,TIMESORT_KEY_OLDEST,CUSTOM_KEY,BACK_KEY,HELP_KEY,END_KEY,NICEPLUS_KEY,NICEMINUS_KEY,
		TOGGLESUSPEND_KEY,KILLKILL_KEY};


	for(int i = 0; i < PS_PP_COLORS; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			if(a[i][j] < 0 || a[i][j] > 1000)
			{
				fprintf(stderr,"configreader: invalid color range (allowed: 0-1000)\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	for(int i = 0; i  < PS_PP_KEYS; i++)
	{
		if(!((b[i] >= 'a' && b[i] <= 'z') || (b[i] >= 'A' && b[i] <= 'Z')))
		{
			fprintf(stderr,"configreader: %d not a bindable key ([A-Z][a-z] only)\n",b[i]);
			exit(EXIT_FAILURE);
		}
	}

	for(int i = 0; i  < PS_PP_KEYS - 1; i++)
	{
		for(int j = i + 1; j < PS_PP_KEYS; j++)
		{
			if(b[i] == b[j])
			{
				fprintf(stderr,"configreader: keys must be unique: \'%c\'\n",b[i]);
				exit(EXIT_FAILURE);
			}
		}
	}
	if(configreader != NULL)
		fclose(configreader);




}