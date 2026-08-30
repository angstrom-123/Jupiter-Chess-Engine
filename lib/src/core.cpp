#include "core.h"

uint8_t ToIndex(uint8_t x, uint8_t y)
{
    return x + 8 * y;
}

uint8_t Difference(uint8_t a, uint8_t b)
{
    if (a > b)
        return a - b;
    return b - a;
}
