#ifndef PARSEOPTS_H
#define PARSEOPTS_H

#define ARGNAME_MAX 200
#define MAXARGS 200
#define MAX_OPTS 100

struct option
{
	char short_name[3];
	char long_name[ARGNAME_MAX];
	char * buffer[MAXARGS];
	int no;

};

struct arg_parse 
{
	struct option * options[MAX_OPTS];
	int option_no;
};


int process_exists(int pid);
void sanitycheck(char ** formatbuffer, int formatlen, char ** formats, int format_no,
				 char ** pidbuffer, int pidlen,
				 char ** userbuffer, int userlen);
int testoption(char * s, struct arg_parse * a, int * optind);
int newargcheck(char * s, struct arg_parse * a, int optind);
void parseopts(int argc, char ** argv, struct arg_parse * argp);

#endif