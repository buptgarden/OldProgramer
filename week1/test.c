// test.c
#include <stdio.h>

void foo(int x) {
    printf("foo: %d\n", x);
}

int main() {
    for (int i = 0; i < 3; i++) {
        foo(i);
    }
    return 0;
}
