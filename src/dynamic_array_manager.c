#include <stdlib.h>
#include <stdio.h>
#include "structures.h"
#include "helper.h"

extern PROCESS_LL * head;

struct final_print_struct *fps;
int fps_size;


void add_final(char * mesg, int pid)
{
    struct final_print_struct new_struct;
    snprintf(new_struct.mesg, sizeof(new_struct.mesg), "%s", mesg);
    new_struct.pid = pid;
    fps_size++;
  	
  	fps = realloc(fps, fps_size * sizeof(struct final_print_struct));

    if (fps == NULL) {
		freeList(head);
		free(fps);
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    fps[fps_size - 1] = new_struct;
}

void clear_and_reset_array(struct final_print_struct **array, int *array_size) {
    free(*array);
    *array_size = 0;
    *array = NULL;
}
