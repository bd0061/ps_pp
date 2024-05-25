#define _GNU_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include "structures.h"
#include "helper.h"

#include "parseopts.h"


/* nakon uspesnog parsiranja argumenata, ova funkcija radi sledece:
 * 1) proverava da li su sve snabdevene proces informacije legitimne(postoje kao opcija)
 * 2) proverava da li su uneti pid brojevi  i da li postoje na sistemu
 * 3) proverava da li uneti korisnici postoje na sistemu
 */
void sanitycheck(char ** formatbuffer, int formatlen, char ** formats, int format_no,
				 char ** pidbuffer, int pidlen,
				 char ** userbuffer, int userlen)
{
	char * t = NULL;
	for(int i = 0; i < formatlen; i++)
	{
		int ok = 0;
		for(int j = 0; j < format_no; j++)
		{
			t = formatbuffer[i];
			if(strcasecmp(formatbuffer[i],formats[j]) == 0)
			{
				ok = 1;
				break;
			}
		}
		if(!ok)
		{
			fprintf(stderr,"Invalid format argument: %s\n",formatbuffer[i]);
			fprintf(stderr,"Exhaustive list of all supported options for formatting:\n[");
			for(int k = 0; k < format_no; k++)
			{
				fprintf(stderr,"%s%s",formats[k], k != format_no - 1 ? "," : "]\n");
			}
			exit(EXIT_FAILURE);
		}
		if(t != NULL)
		{
			while(*t)
			{
				*t = toupper(*t);
				t++;
			}
		}

	}

	for(int k = 0; k < pidlen; k++)
	{
		if(!num(pidbuffer[k]))
		{
			fprintf(stderr,"Invalid pid argument: %s\n",pidbuffer[k]);
			exit(EXIT_FAILURE);
		}
	}

	int user_no = 0;
	char *users[1024];

	struct passwd *pw;
	setpwent();
	
	while ((pw = getpwent()) != NULL) {
		users[user_no++] = strdup(pw->pw_name);
    }


	for(int m = 0; m < userlen; m++)
	{
		int ok_usr = 0;
		for(int z = 0; z < user_no; z++)
		{
			if(strcmp(userbuffer[m],users[z]) == 0)
			{
				ok_usr = 1;
				break;
			}
		}
		if(!ok_usr)
		{
			fprintf(stderr,"Invalid user: %s\n",userbuffer[m]);
			exit(EXIT_FAILURE);
		}
	}
	
	for(int i = 0; i < user_no; i++)
	{
		free(users[i]);
	}
	
	endpwent();


}


static int testoption(char * s, struct arg_parse * a, int * optind)
{
	for(int i = 0; i < a->option_no; i++)
	{
		if(strcmp(s,(a->options)[i]->short_name) == 0 || strcmp(s,(a->options)[i]->long_name) == 0)
		{
			*optind = i;
			return 0;
		}
	}
	return -1;
}

static int newargcheck(char * s, struct arg_parse * a)
{
	for(int i = 0; i < a->option_no; i++)
	{		
		if((strcmp(s,(a->options)[i]->short_name) == 0 || strcmp(s,(a->options)[i]->long_name) == 0))
		{
			return 1;
		}

	}
	return 0;
}

/* algoritam za parsiranje argumenata

*/
void parseopts(char ** argv, struct arg_parse * argp)
{
	++argv; //preskoci ime samog programa
	if(*argv == NULL)
	{
		return;
	}
	parseout: ;
	
	int optind;
	if(testoption(*argv,argp,&optind) == -1)
	{
		//moze da se desi samo prvi put
		fprintf(stderr,"Invalid %s: %s\n", (*argv)[0] == '-' ? "option" : "argument", *argv);
		exit(EXIT_FAILURE);
	}
	//u suprotnom, u optind je indeks structa koji odgovara nasem switchu
	
	char ** emptycheck = argv;
	++emptycheck;
	//da li je ovo poslednja opcija u streamu?
	if(*emptycheck == NULL)
	{
		//ako jeste, u pitanju je greska jer svaka opcija zahteva bar 1 arguemnt
		fprintf(stderr,"Option %s takes at least one argument\n",*argv);
		exit(EXIT_FAILURE);
	}

	
	int countargs = 0;
	char * option = *argv; //sacuvati string opcije 
	++argv;
	while(*argv != NULL)
	{
		if(newargcheck(*argv,argp)) //da li smo naisli na novu opciju?
		{
			if(countargs == 0) // vec novi switch bez bar jednog argumenta za prethodni
			{
				fprintf(stderr,"Option %s takes at least one argument\n",option);
				exit(EXIT_FAILURE);
			}
			
			//ako je bio vec jedan arg, ponoviti ceo proces za novi switch
			goto parseout; // u *argv je trenutno novi switch
		}
		else if((*argv)[0] == '-') //switch koji se ne nalazi u nasem structu
		{
			fprintf(stderr,"Invalid %s: %s\n", (*argv)[0] == '-' ? "option" : "argument", *argv);
			exit(EXIT_FAILURE);
		}
		
		//skladisti novi argument za trenutnu opciju
		((argp->options)[optind])->buffer[((argp->options)[optind]->no)++] = *argv; //lol
		countargs++;
		argv++;
	}

	
}