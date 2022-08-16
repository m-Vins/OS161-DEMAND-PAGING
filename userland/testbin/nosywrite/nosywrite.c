#include <stdio.h>


int main(void){
    int *invalid_pointer = (int *)0x400000; /*this address belongs to the text segment, which is read-only */
    *invalid_pointer = 3;
    return 0;
}