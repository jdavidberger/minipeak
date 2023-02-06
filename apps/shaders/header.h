#undef MAD_4
#undef MAD_16
#undef MAD_64

#define mad(a,b,c) (a*b+c)
#define MAD_4(x, y)     x = mad(y, x, y);   y = mad(x, y, x);   x = mad(y, x, y);   y = mad(x, y, x);
#define MAD_16(x, y)    MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);
#define MAD_64(x, y)    MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);

struct vec8 {
    vec4 d0, d1;
};
#define VEC8(x0,x1,x2,x3,x4,x5,x6,x7) vec8(vec4(x0,x1,x2,x3), vec4(x4,x5,x6,x7))
#define VEC8_S(x) vec8(vec4(x,x,x,x), vec4(x,x,x,x))


#define VEC8_ADD(a, b) (vec8(a.d0 + b.d0, a.d1 + b.d1))
#define VEC8_MUL(a, b) (vec8(a.d0 * b.d0, a.d1 * b.d1))

struct vec16 {
    vec8 d0,d1;
};

#define VEC16(x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15) vec16(VEC8(x0,x1,x2,x3,x4,x5,x6,x7), VEC8(x8,x9,x10,x11,x12,x13,x14,x15))
#define VEC16_S(x) vec16(VEC8_S(x), VEC8_S(x));

#define VEC16_ADD(a, b) (vec16(VEC8_ADD(a.d0, b.d0), VEC8_ADD(a.d1, b.d1)))
#define VEC16_MUL(a, b) (vec16(VEC8_MUL(a.d0, b.d0), VEC8_MUL(a.d1, b.d1)))

#define mad8(a,b,c) (VEC8_ADD(VEC8_MUL(a,b),c))
#define mad16(a,b,c) (VEC16_ADD(VEC16_MUL(a,b),c))

layout(local_size_x = 256) in;

layout(std430, binding = 0) restrict writeonly buffer raw9i {
        float ptr[];
};

layout(binding = 1) uniform _A_block {  float _A; };

#define SCALE 1e-10