#include "defines.h"
#include "serial.h"
#include "lib.h"

volatile int value = 10;

int main() {
    serial_init(SERIAL_DEFAULT_DEVICE);

    puts("Hello World!\n");

    putxval(value, 0);
    puts("\n");
    value = 20;
    putxval(value, 0);
    puts("\n");

    while(1);

    return 0;
}