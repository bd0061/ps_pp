#ifndef DYNAMIC_ARRAY_MANAGER_H
#define DYNAMIC_ARRAY_MANAGER_H

extern struct final_print_struct *fps;
extern int fps_size;

void add_final(char * mesg, int pid, char s);
void clear_and_reset_array(struct final_print_struct **array, int *array_size);


#endif