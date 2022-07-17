#include <stdio.h>

struct person {
    char name;
    int32_t age;
    char address;
};

#define beijing ="北京天安门";

const char shanghai[]="北京天安门";

extern int x;
extern int y;

int add() {
    return x + y;
}

int main() {
    x = 0;
    y = 1;
    int result = add();
    printf(result);
    printf("C语言中文网");
}
