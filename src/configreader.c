#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
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

#define PS_PP_COLORS 7
#define PS_PP_KEYS 9



void readvals()
{
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
	int CUSTOM_KEY_CHECK = 0;

	char * home = getenv("HOME");

	if(home == NULL)
	{
		fprintf(stderr,"Couldn't read home environment variable\n");
		exit(EXIT_FAILURE);
	}

	char fullpath[1024];
	snprintf(fullpath,sizeof(fullpath),"%s/.config/ps_pp.conf",home);

	FILE * configreader = fopen(fullpath,"r");

	if(configreader == NULL)
	{
		if(errno == ENOENT)
		{
			FILE * configskeleton = fopen(fullpath,"w");
			if(configskeleton != NULL)
			{
				fprintf(configskeleton,
				"#color format: RGB(values 0-1000)\n"
				"#key format: single character([A-Z][a-z])\n"
				"#available variables:\n\n"
				"#COLOR_BG=(R,G,B)\n"
				"#COLOR_TEXT=(R,G,B)\n"
				"#COLOR_PRCNT_BAR=(R,G,B)\n"
				"#COLOR_SELECTPROC=(R,G,B)\n"
				"#COLOR_INFOTEXT=(R,G,B)\n"
				"#COLOR_INFO_SUC=(R,G,B)\n"
				"#COLOR_INFO_ERR=(R,G,B)\n"
				"#QUIT_KEY=K\n"
				"#KILL_KEY=K\n"
				"#IO_KEY=K\n"
				"#ID_KEY=K\n"
				"#CLASSIC_KEY=K\n"
				"#MEMSORT_KEY=K\n"
				"#CPUSORT_KEY=K\n"
				"#NORMALSORT_KEY=K\n"	
				"#CUSTOM_KEY=K\n"		
				);
				
				fclose(configskeleton);
			}
			goto defaultcolor;
		}
		perror("configreader: fopen");
		exit(EXIT_FAILURE);
	}

	char line[1024];
	while(fgets(line,sizeof(line),configreader) != NULL)
	{
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
	 	COLOR_INFOTEXT[0] = 400; COLOR_INFOTEXT[1] = 400; COLOR_INFOTEXT[2] = 1000;
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
	 
	 if(!CUSTOM_KEY_CHECK)
	 	CUSTOM_KEY = 'x';

	int * a[PS_PP_COLORS] = {COLOR_BG,COLOR_TEXT,COLOR_PRCNT_BAR,COLOR_SELECTPROC,COLOR_INFOTEXT,COLOR_INFO_SUC,COLOR_INFO_ERR};

	char b[PS_PP_KEYS] = {QUIT_KEY,KILL_KEY,IO_KEY,ID_KEY,CLASSIC_KEY,MEMSORT_KEY,CPUSORT_KEY,NORMALSORT_KEY,CUSTOM_KEY};


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
			fprintf(stderr,"configreader: b[%d]=%d not a bindable key ([A-Z][a-z] only)\n",i,b[i]);
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
	
	if(configreader)
		fclose(configreader);




}