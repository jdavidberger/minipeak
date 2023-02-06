#include "header.h"
#line 3

#undef mad
#define mad mad16

void main()
{
    uint id = gl_GlobalInvocationID[0] + gl_GlobalInvocationID[1] * 256u + gl_GlobalInvocationID[2] * 256u * 256u;
//    vec4 x = vec4(_A, (_A+1.f), (_A+2.f), (_A+3.f));
//    vec4 y = vec4(id, id, id, id);
//
//    for(int i=0; i<32; i++)
//    {
//        MAD_16(x, y);
//    }
//
//    ptr[id] = (y.x) + (y.y) + (y.z) + (y.w);
    vec16 x = VEC16(_A, (_A+1.0f), (_A+2.0f), (_A+3.0f), (_A+4.0f), (_A+5.0f), (_A+6.0f), (_A+7.0f),
    (_A+8.0f), (_A+9.0f), (_A+10.0f), (_A+11.0f), (_A+12.0f), (_A+13.0f), (_A+14.0f), (_A+15.0f));
    vec16 y = VEC16_S(float(id) * SCALE);

    for(int i=0; i<8; i++)
    {
        MAD_16(x, y);
    }

    vec8 u = VEC8_ADD(y.d0, y.d1);
    vec4 s = u.d0 + u.d1;
    vec2 t = s.xy + s.zw;
    ptr[id] = t.x + t.y;
}
