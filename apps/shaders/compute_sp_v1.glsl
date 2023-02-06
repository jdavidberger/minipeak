#include "header.h"
#line 3

void main()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    float x = _A;
    float y = float(id) * SCALE;

    for(int i=0; i<128; i++)
    {
        MAD_16(x, y);
    }

    ptr[id] = y;
}