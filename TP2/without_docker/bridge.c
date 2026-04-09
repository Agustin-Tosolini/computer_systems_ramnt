#include "cdecl.h"
#include <stdio.h>

int PRE_CDECL convert( float ratio ) POST_CDECL;

void bridge(int size, float* ratios, int* result)
{
    
    for(int i = 0; i < size; i++)
    {
        result[i] = convert(ratios[i]);
    }
}

/*
// Example of usage of the bridge function in C
int main(void)
{
    float ratios[] = {1.2, 2.7, 3.9};
    int size = sizeof(ratios) / sizeof(ratios[0]);
    int result[size];

    for(int i = 0; i < size; i++)
    {
        result[i] = 0; // Initialize the result array
    }

    bridge(size, ratios, result);

    for(int i = 0; i < size; i++)
    {
        printf("Ratio: %f, Converted with addition 1: %d\n", ratios[i], result[i]);
    }
    return 0;
}
*/