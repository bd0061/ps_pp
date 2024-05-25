#ifndef COMPONENTS_H
#define COMPONENTS_H

#define LOGO_HEIGHT 6

#ifdef FORMAT_TABLE
	#define tabbord (i == buflength - 1 ? "|" : "")
	#define tabstart (i == 0 ? "|" : "")
#else 
	#define tabbord (i == buflength - 1 ? " " : "")
	#define tabstart (i == 0 ? " " : "")
#endif

#define INFOMSG_POSITION 15 - NOLOGO * (LOGO_HEIGHT - 1)
#define FILTERMSG_POSITION 16 - NOLOGO * (LOGO_HEIGHT - 1)

#define LAG 10 //ms
#define REFRESH_RATE 1000 //ms

extern char INFOMSG[65];
extern int GLOBAL_CURSE_OFFSET;
extern int LIST_START;
extern int countDown;
extern int SUCCESS;
extern int FILTERING;
extern int selectedLine;
extern int x,y,cursx,cursy;
extern int start_pspp;


extern int formatvals[35];
extern char NAME_FILTER[256];



void curse_init();
void printhelpmenu();
void handle_io(char * dest, size_t destsize, long long target, int blocked, int pers);
void prettyprint(char * s, int len);
void print_stats();
void print_art();
void print_header(char ** buffer, int buflength, char ** formats, int format_no, int * printno_export);
void collect_data(char ** buffer, int buflength, PROCESS_LL * start);
void print_upper_menu_and_mod_offset(char ** fbuf, int fno, char ** dformats, int dformatno, char ** formats, int format_no, int * printno_export);
void refreshList();
void displayScreen(char ** fbuf, int fno, char ** dformats, int dformatno, char ** formats, int format_no, int * printno_export);

void start_main_ncurses_loop();

#endif // COMPONENTS_H