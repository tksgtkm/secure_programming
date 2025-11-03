#include <stdint.h>

#include "hypercall_x86_64.h"
#include "vm.h"

char stack[8192];

void start_kernel(start_info_t *start_info) {
    HYPERVISOR_console_io(CONSOLE_write, 12, "Hello World\n");
    while(1);
}