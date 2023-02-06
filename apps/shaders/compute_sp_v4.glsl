#include "header.h"
#line 3

void main()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    vec4 x = vec4(_A, (_A+1.f), (_A+2.f), (_A+3.f));
    vec4 y = vec4(id, id, id, id) * SCALE;

    for(int i=0; i<32; i++)
    {
        MAD_16(x, y);
    }

    ptr[id] = (y.x) + (y.y) + (y.z) + (y.w);
}
