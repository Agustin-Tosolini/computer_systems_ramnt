#include <stddef.h>

extern int asm_convert_float_array_to_int(const float *input, int *output, int count);

int convert_float_array_to_int(const float *input, int *output, int count) {
    if (input == NULL || output == NULL || count < 0) {
        return -1;
    }

    return asm_convert_float_array_to_int(input, output, count);
}
