#include <stdio.h>
#include <stdlib.h>

int ntz(unsigned x) {
    int n;

    if (x == 0)
        return 32;
    n = 1;
    if ((x & 0x0000FFFF) == 0) {
        n = n + 16;
        x = x >> 16;
    }
    if ((x & 0x000000FF) == 0) {
        n = n + 8;
        x = x >> 8;
    }
    if ((x & 0x0000000F) == 0) {
        n = n + 4;
        x = x >> 4;
    }
    if ((x & 0x00000003) == 0) {
        n = n + 2;
        x = x >> 2;
    }

    return n - (x & 1);
}

int nlz(unsigned x) {
    int n;

    if (x == 0)
        return 32;
    n = 0;
    if (x <= 0x0000FFFF) {
        n = n + 16;
        x = x << 16;
    }
    if (x <= 0x00FFFFFF) {
        n = n + 8;
        x = x << 8;
    }
    if (x <= 0x0FFFFFFF) {
        n = n + 4;
        x = x << 4;
    }
    if (x <= 0x3FFFFFFF) {
        n = n + 2;
        x = x << 2;
    }
    if (x <= 0x7FFFFFFF) {
        n = n + 1;
    }

    return n;
}

int pop(unsigned x) {
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    x = x + (x << 8);
    x = x + (x << 16);
    return x >> 24;
}

unsigned snoob(unsigned x) {
    unsigned smallest, ripple, ones;

    smallest = x & -x;
    ripple = x + smallest;
    ones = x ^ ripple;
    ones = (ones >> 2)/smallest;
    return ripple | ones;
}

unsigned snoob1(unsigned x) {
    unsigned smallest, ripple, ones;

    smallest = x & -x;
    ripple = x + smallest;
    ones = x ^ ripple;
    ones = ones >> 2;
    ones = ones >> ntz(x);
    return ripple | ones;
}

unsigned snoob2(unsigned x) {
    unsigned smallest, ripple, ones;

    smallest = x & -x;
    ripple = x + smallest;
    ones = x ^ ripple;
    ones = ones >> (33 - nlz(smallest));
    return ripple | ones;
}

unsigned snoob3(unsigned x) {
    unsigned smallest, ripple, ones;

    smallest = x & -x;
    ripple = x + smallest;
    ones = x ^ ripple;
    ones = (1 << (pop(ones) - 2)) - 1;
    return ripple | ones;
}

unsigned next_set_of_n_elements(unsigned x) {
    unsigned smallest, ripple, new_smallest, ones;

    if (x == 0)
        return 0;
    smallest = (x & -x);
    ripple = x + smallest;
    new_smallest = (ripple & -ripple);
    ones = ((new_smallest / smallest) >> 1) - 1;
    return ripple | ones;
}

int snoob4(int x) {
    int y = x + (x & -x);
    x = x & ~y;
    while ((x & 1) == 0)
        x = x >> 1;
    x = x >> 1;
    return y | x;
}

int main(int argc, char *argv[]) {
    int n;
    unsigned x, y, z, u, v, w;

    if (argc != 2) {
        printf("Need exactly one argument, an integer from 1 to 7.\n");
        exit(1);
    }

    n = strtol(argv[1], NULL, 10);
    if (n < 1 || n > 7) {
        printf("Argument must be an integer from 1 to 7.\n");
        exit(1);
    }

    printf("n = %d\n", n);

    x = (1 << n) - 1;
    y = x;
    z = x;
    u = x;
    v = x;
    w = x;
    do {
        printf("x, y, z, u, v, w = %02X %02X %02X %02X %02X %02X\n", x, y, z, u, v, w);
        y = snoob1(x);
        z = snoob2(x);
        u = snoob3(x);
        v = next_set_of_n_elements(x);
        w = snoob4(x);
        x = snoob(x);
    } while (x <= 255);

    return 0;
}