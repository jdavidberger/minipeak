#line 1002

#define MINIPEAK_IDX_CWH(c,x,y,channels,width,height) (uint(uint(c) * ((width) * (height)) + uint(x) * (width) + uint(y)))
#define MINIPEAK_IDX_0 MINIPEAK_IDX_CWH

#define MINIPEAK_IDX_WHC(c,x,y,channels,width,height) (uint(uint(x) * ((width) * (channels)) + uint(y) * (channels) + uint(c)))
#define MINIPEAK_IDX_1 MINIPEAK_IDX_wHC

#define MINIPEAK_IDX(c,x,y,order,channels,width,height) (order == 1 ? MINIPEAK_IDX_WHC(c,x,y,channels,width,height) : MINIPEAK_IDX_CWH(c,x,y,channels,width,height))

#define MINIPEAK_ACCESS_NATIVE(d,c,x,y,orders,channels,width,height) d[MINIPEAK_IDX(c,x,y,orders,channels,width,height)]
#define MINIPEAK_ACCESS_b MINIPEAK_ACCESS_NATIVE
#define MINIPEAK_ACCESS_f MINIPEAK_ACCESS_NATIVE
#define MINIPEAK_ACCESS_f2 MINIPEAK_ACCESS_NATIVE
#define MINIPEAK_ACCESS_f3 MINIPEAK_ACCESS_NATIVE
#define MINIPEAK_ACCESS_f4 MINIPEAK_ACCESS_NATIVE
#define MINIPEAK_ACCESS_d MINIPEAK_ACCESS_NATIVE
#define MINIPEAK_ACCESS_h MINIPEAK_ACCESS_NATIVE
#define MINIPEAK_ACCESS_i MINIPEAK_ACCESS_NATIVE

#define MINIPEAK_ACCESS_1D_NATIVE(d,idx) d[idx]
#define MINIPEAK_ACCESS_1D_f MINIPEAK_ACCESS_1D_NATIVE
#define MINIPEAK_ACCESS_1D_f2 MINIPEAK_ACCESS_1D_NATIVE
#define MINIPEAK_ACCESS_1D_f4 MINIPEAK_ACCESS_1D_NATIVE

#define MINIPEAK_PACKED_16_2_DTYPE int
#define MINIPEAK_PACKED_16_4_DTYPE ivec2

#define MINIPEAK_EXPAND_PACKED_16_4(x) ivec4(uint(x[0]) & 0xffffu, (uint((x)[0]) >> 16u) & 0xffffu, uint(x[1]) & 0xffffu, (uint((x)[1]) >> 16u) & 0xffffu)
#define MINIPEAK_EXPAND_PACKED_16_2(x) ivec2(uint(x) & 0xffffu, (uint((x)) >> 16u) & 0xffffu)

#define MINIPEAK_ACCESS_1D_s4(d,idx) MINIPEAK_EXPAND_PACKED_16_4(d[idx])
#define MINIPEAK_ACCESS_1D_s2(d,idx) MINIPEAK_EXPAND_PACKED_16_2(d[idx])

#define MINIPEAK_ACCESS_s4(d,c,x,y,orders,channels,width,height) MINIPEAK_EXPAND_PACKED_16_4(d[MINIPEAK_IDX(c,x,y,orders,channels,width,height)/2u])
#define MINIPEAK_ACCESS_s2(d,c,x,y,orders,channels,width,height) MINIPEAK_EXPAND_PACKED_16_2(d[MINIPEAK_IDX(c,x,y,orders,channels,width,height)/2u])

#ifdef GL_EXT_shader_16bit_storage
#define MINIPEAK_ACCESS_s MINIPEAK_ACCESS_NATIVE
#else
#define MINIPEAK_ACCESS16(d,idx) ((d[(idx)/2u] >> (16u*((idx)%2u))) & 0xffff)
#define MINIPEAK_ACCESS_s(d,c,x,y,orders,channels,width,height) MINIPEAK_ACCESS16(d, MINIPEAK_IDX(c,x,y,orders,channels,width,height))
//#define MINIPEAK_ACCESS_s2(d,c,x,y,orders,channels,width,height) MINIPEAK_ACCESS16(d, MINIPEAK_IDX(c,x,y,orders,channels,width,height))
#endif
 
#define MINIPEAK_STORE_NATIVE(d,c,x,y,orders,channels,width,height,v) d[MINIPEAK_IDX(c,x,y,orders,channels,width,height)] = (v)
#define MINIPEAK_STORE_b MINIPEAK_STORE_NATIVE
#define MINIPEAK_STORE_f MINIPEAK_STORE_NATIVE
#define MINIPEAK_STORE_f2 MINIPEAK_STORE_NATIVE
#define MINIPEAK_STORE_f4 MINIPEAK_STORE_NATIVE
#define MINIPEAK_STORE_d MINIPEAK_STORE_NATIVE
#define MINIPEAK_STORE_h MINIPEAK_STORE_NATIVE

#define MINIPEAK_STORE_1D_NATIVE(d,idx,v) d[idx] = v
#define MINIPEAK_STORE_1D_f4(d,idx,v) d[idx] = vec4(v)
#define MINIPEAK_STORE_1D_f2(d,idx,v) d[idx] = vec2(v)
#define MINIPEAK_STORE_1D_f(d,idx,v) d[idx] = float(v)


#define MINIPEAK_COMPRESS_PACKED_16_2(x) ((x[1] & 0xffff) | ((x[0] << 16u)) & 0xffff)
#define MINIPEAK_COMPRESS_PACKED_16_4(x) ivec2(MINIPEAK_COMPRESS_PACKED_16_2(ivec2(x[0],x[1])),\
					       MINIPEAK_COMPRESS_PACKED_16_2(ivec2(x[2],x[3])))

#define MINIPEAK_STORE_1D_s4(d,idx,v) d[idx] = MINIPEAK_COMPRESS_PACKED_16_4((v))
#define MINIPEAK_STORE_1D_s2(d,idx,v) d[idx] = MINIPEAK_COMPRESS_PACKED_16_2((v))

#define MINIPEAK_STORE_s4(d,c,x,y,orders,channels,width,height,v) d[MINIPEAK_IDX(c,x,y,orders,channels,width,height)/2u] = MINIPEAK_COMPRESS_PACKED_16_4(ivec4(v))
#define MINIPEAK_STORE_s2(d,c,x,y,orders,channels,width,height,v) d[MINIPEAK_IDX(c,x,y,orders,channels,width,height)/2u] = MINIPEAK_COMPRESS_PACKED_16_2(ivec2(v))

#define MINIPEAK_CIDX_TO_1D_IDX(c,idx,orders,channels,width,height) ((c) * (width * height) + (idx))
