#include <stdio.h>
#include <stdlib.h>
#include "MemoryError.h"

/* Alerts of a failed memory allocation, and exits with error code 1. */
void report_memory_allocation_error(char *function_name) {
    printf("Error: function \"%s\" has failed\n", function_name);
    exit(1);
}

void validate_memory_allocation(char *function_name, void *p) {
    if (p == NULL) {
        report_memory_allocation_error(function_name);
    }
}


