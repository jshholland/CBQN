#include "../core.h"
#include "../utils/each.h"
#include "../builtins.h"
#include "../nfns.h"
#include "../ns.h"
#include <math.h>

static inline B arith_recm(BB2B f, B x) {
  B fx = getFillQ(x);
  B r = eachm_fn(bi_N, x, f);
  return withFill(r, fx);
}

B bit_negate(B x) { // consumes
  u64* xp = bitarr_ptr(x);
  u64* rp; B r = m_bitarrc(&rp, x);
  usz ia = BIT_N(IA(x));
  for (usz i = 0; i < ia; i++) rp[i] = ~xp[i];
  decG(x);
  return r;
}

#define GC1i(SYMB,NAME,FEXPR,IBAD,IEXPR,BX,SQF) B NAME##_c1(B t, B x) { \
  if (isF64(x)) { f64 v = x.f; return m_f64(FEXPR); }                   \
  if (RARE(!isArr(x))) thrM(SYMB ": Expected argument to be a number"); \
  u8 xe = TI(x,elType);                                                 \
  i64 sz = IA(x); BX                                                    \
  if (xe==el_i8) { i8 MAX=I8_MAX; i8 MIN=I8_MIN; i8* xp=i8any_ptr(x); i8* rp; B r=m_i8arrc(&rp,x);        \
    for (i64 i = 0; i < sz; i++) { i8 v = xp[i]; if (RARE(IBAD)) { decG(r); goto base; } rp[i] = IEXPR; } \
    decG(x); (void)MIN;(void)MAX; return r;                             \
  }                                                                     \
  if (xe==el_i16) { i16 MAX=I16_MAX; i16 MIN=I16_MIN; i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrc(&rp,x); \
    for (i64 i = 0; i < sz; i++) { i16 v = xp[i]; if (RARE(IBAD)) { decG(r); goto base; } rp[i] = IEXPR; }  \
    decG(x); (void)MIN;(void)MAX; return r;                             \
  }                                                                     \
  if (xe==el_i32) { i32 MAX=I32_MAX; i32 MIN=I32_MIN; i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrc(&rp,x); \
    for (i64 i = 0; i < sz; i++) { i32 v = xp[i]; if (RARE(IBAD)) { decG(r); goto base; } rp[i] = IEXPR; }  \
    decG(x); (void)MIN;(void)MAX; return r;                             \
  }                                                                     \
  if (xe==el_f64) { f64* xp = f64any_ptr(x);                            \
    f64* rp; B r = m_f64arrc(&rp, x);                                   \
    for (i64 i = 0; i < sz; i++) { f64 v = xp[i]; rp[i] = FEXPR; }      \
    decG(x); return SQF? num_squeeze(r) : r;                            \
  }                                                                     \
  base: SLOW1(SYMB"𝕩", x); return arith_recm(NAME##_c1, x);             \
}
  

B add_c1(B t, B x) {
  if (isF64(x)) return x;
  if (!isArr(x)) thrM("+: Argument must consist of numbers");
  if (elNum(TI(x,elType))) return x;
  dec(eachm_fn(m_f64(0), inc(x), add_c1));
  return x;
}

GC1i("-", sub,   -v,              v== MIN, -v,      {}, 0) // change icond to v==-v to support ¯0 (TODO that won't work for i8/i16)
GC1i("|", stile, fabs(v),         v== MIN, v<0?-v:v,{}, 0)
GC1i("⌊", floor, floor(v),        0,           v,   {}, 1)
GC1i("⌈", ceil,  ceil(v),         0,           v,   {}, 1)
GC1i("×", mul,   v==0?0:v>0?1:-1, 0,           v==0?0:v>0?1:-1,{}, 1)
GC1i("¬", not,   1-v,             v<=-MAX, 1-v,     {
  if(xe==el_bit) return bit_negate(x);
}, 0)

#define P1(N) { if(isArr(x)) { SLOW1("arithm " #N, x); return arith_recm(N##_c1, x); } }
B   div_c1(B t, B x) { if (isF64(x)) return m_f64(    1/x.f ); P1(  div); thrM("÷: Getting reciprocal of non-number"); }
B   pow_c1(B t, B x) { if (isF64(x)) return m_f64(  exp(x.f)); P1(  pow); thrM("⋆: Getting exp of non-number"); }
B  root_c1(B t, B x) { if (isF64(x)) return m_f64( sqrt(x.f)); P1( root); thrM("√: Getting root of non-number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f)); P1(  log); thrM("⋆⁼: Getting log of non-number"); }
B   sin_c1(B t, B x) { if (isF64(x)) return m_f64(  sin(x.f)); P1(  sin); thrM("•math.Sin: Argument contained non-number"); }
B   cos_c1(B t, B x) { if (isF64(x)) return m_f64(  cos(x.f)); P1(  cos); thrM("•math.Cos: Argument contained non-number"); }
B   tan_c1(B t, B x) { if (isF64(x)) return m_f64(  tan(x.f)); P1(  tan); thrM("•math.Tan: Argument contained non-number"); }
B  asin_c1(B t, B x) { if (isF64(x)) return m_f64( asin(x.f)); P1( asin); thrM("•math.Asin: Argument contained non-number"); }
B  acos_c1(B t, B x) { if (isF64(x)) return m_f64( acos(x.f)); P1( acos); thrM("•math.Acos: Argument contained non-number"); }
B  atan_c1(B t, B x) { if (isF64(x)) return m_f64( atan(x.f)); P1( atan); thrM("•math.Atan: Argument contained non-number"); }
#undef P1

B lt_c1(B t, B x) { return m_atomUnit(x); }
B eq_c1(B t, B x) { if (isAtm(x)) { decA(x); return m_i32(0); } B r = m_i32(RNK(x)); decG(x); return r; }
B ne_c1(B t, B x) { if (isAtm(x)) { decA(x); return m_i32(1); } B r = m_f64(*SH(x)); decG(x); return r; }


static B mathNS;
B getMathNS() {
  if (mathNS.u == 0) {
    #define F(X) inc(bi_##X),
    Body* d = m_nnsDesc("sin","cos","tan","asin","acos","atan");
    mathNS = m_nns(d,  F(sin)F(cos)F(tan)F(asin)F(acos)F(atan));
    #undef F
    gc_add(mathNS);
  }
  return inc(mathNS);
}

void arith_init() {
  c(BFn,bi_add)->ident = c(BFn,bi_sub)->ident = c(BFn,bi_or )->ident = c(BFn,bi_ne)->ident = c(BFn,bi_gt)->ident = m_i32(0);
  c(BFn,bi_mul)->ident = c(BFn,bi_div)->ident = c(BFn,bi_and)->ident = c(BFn,bi_eq)->ident = c(BFn,bi_ge)->ident = c(BFn,bi_pow)->ident = c(BFn,bi_not)->ident = m_i32(1);
  c(BFn,bi_floor)->ident = m_f64(1.0/0.0);
  c(BFn,bi_ceil )->ident = m_f64(-1.0/0.0);
  
  c(BFn,bi_sub)->im = sub_c1;
  c(BFn,bi_sin)->im = asin_c1;
  c(BFn,bi_cos)->im = acos_c1;
  c(BFn,bi_tan)->im = atan_c1;
  c(BFn,bi_asin)->im = sin_c1;
  c(BFn,bi_acos)->im = cos_c1;
  c(BFn,bi_atan)->im = tan_c1;
}
