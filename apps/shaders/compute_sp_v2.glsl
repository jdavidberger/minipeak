#include "header.h"
#line 3

void main()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    vec2 x = vec2(_A, (_A+1.f));
    vec2 y = vec2(id, id) * SCALE;

    for(int i=0; i<64; i++)
    {
        MAD_16(x, y);
    }

    ptr[id] = (y.x) + (y.y);
}
