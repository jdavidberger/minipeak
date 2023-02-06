#include "header.h"
#line 3


#undef mad
#define mad mad8

void main()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
    vec8 x = VEC8(_A, (_A+1.0f), (_A+2.0f), (_A+3.0f), (_A+4.0f), (_A+5.0f), (_A+6.0f), (_A+7.0f));
    vec8 y = VEC8_S(float(id) * SCALE);

    for(int i=0; i<16; i++)
    {
        MAD_16(x, y);
    }

    vec4 s = y.d0 + y.d1;
    vec2 t = s.xy + s.zw;
    ptr[id] = t.x + t.y;
}


