#include <algorithm>
#include <math.h>

#include "utils.h"

float Utils_Sin(float arg)
{
    return sinf(arg);
}

void Utils_CreateRandomPermutation(int* begin, int count)
{
    for (int i=0; i<count; i++)
    {
        begin[i] = i;
    }
    std::random_shuffle(begin, begin+count);
}
