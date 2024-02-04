#include "regex_module.h"
#include <stdio.h>

regex_t regex;

int compile_regex() {
    const char *pattern = "^[0-9]\\{1,\\}$"; 

    int ret = regcomp(&regex, pattern, 0);
    if (ret != 0) {
        char error_buffer[100];
        regerror(ret, &regex, error_buffer, sizeof(error_buffer));
        fprintf(stderr, "Regex compilation failed: %s\n", error_buffer);
        return 1; 
    }

    return 0; 
}