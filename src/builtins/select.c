#include "../core.h"
#include "../utils/talloc.h"
#include "../utils/mut.h"
#include "../builtins.h"

// #if SINGELI
//   #pragma GCC diagnostic push
//   #pragma GCC diagnostic ignored "-Wunused-variable"
//   #include "../singeli/gen/select.c"
//   #pragma GCC diagnostic pop
// #endif

extern B rt_select;
B select_c1(B t, B x) {
  if (isAtm(x)) thrM("⊏: Argument cannot be an atom");
  ur xr = RNK(x);
  if (xr==0) thrM("⊏: Argument cannot be rank 0");
  if (SH(x)[0]==0) thrF("⊏: Argument shape cannot start with 0 (%H ≡ ≢𝕩)", x);
  usz ia = shProd(SH(x), 1, xr);
  Arr* r = TI(x,slice)(incG(x), 0, ia);
  usz* sh = arr_shAlloc(r, xr-1);
  if (sh) shcpy(sh, SH(x)+1, xr-1);
  decG(x);
  return taga(r);
}
B select_c2(B t, B w, B x) {
  if (isAtm(x)) thrM("⊏: 𝕩 cannot be an atom");
  ur xr = RNK(x);
  if (isAtm(w)) {
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    usz csz = arr_csz(x);
    usz cam = SH(x)[0];
    usz wi = WRAP(o2i64(w), cam, thrF("⊏: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam));
    Arr* r = TI(x,slice)(incG(x), wi*csz, csz);
    usz* sh = arr_shAlloc(r, xr-1);
    if (sh) shcpy(sh, SH(x)+1, xr-1);
    decG(x);
    return taga(r);
  }
  B xf = getFillQ(x);
  SGet(x)
  usz wia = IA(w);
  B r;
  
  if (xr==1) {
    if (wia==0) {
      decG(x);
      if (RNK(w)==1) {
        if (isNum(xf)) { r = emptyIVec(); goto ret; }
        if (isC32(xf)) { r = emptyCVec(); goto ret; }
      }
      Arr* ra;
      if (isNum(xf) || isC32(xf)) {
        ra = m_arr(sizeof(TyArr), isNum(xf)? t_i8arr : t_c8arr, 0);
      } else {
        ra = m_fillarrp(0);
        fillarr_setFill(ra, xf);
      }
      arr_shCopy(ra, w);
      r = taga(ra);
      ret:
      decG(w);
      return r;
    }
    usz xia = IA(x);
    u8 xe = TI(x,elType);
    u8 we = TI(w,elType);
    #if SINGELI
      // if (we==el_i8  && xe==el_i32) { i32* rp; r = m_i32arrc(&rp, w); if (!avx2_select_i8_32 ((u8*)i8any_ptr (w), (u8*)i32any_ptr(x), (u8*)rp, wia, xia)) thrM("⊏: Indexing out-of-bounds"); goto dec_ret; }
      // if (we==el_i16 && xe==el_i32) { i32* rp; r = m_i32arrc(&rp, w); if (!avx2_select_i16_32((u8*)i16any_ptr(w), (u8*)i32any_ptr(x), (u8*)rp, wia, xia)) thrM("⊏: Indexing out-of-bounds"); goto dec_ret; }
      // if (we==el_i32 && xe==el_i8 ) { i8*  rp; r = m_i8arrc (&rp, w); if (!avx2_select_i32_8 ((u8*)i32any_ptr(w), (u8*)i8any_ptr (x), (u8*)rp, wia, xia)) thrM("⊏: Indexing out-of-bounds"); goto dec_ret; }
      // if (we==el_i32 && xe==el_i32) { i32* rp; r = m_i32arrc(&rp, w); if (!avx2_select_i32_32((u8*)i32any_ptr(w), (u8*)i32any_ptr(x), (u8*)rp, wia, xia)) thrM("⊏: Indexing out-of-bounds"); goto dec_ret; }
      // if (we==el_i32 && xe==el_f64) { f64* rp; r = m_f64arrc(&rp, w); if (!avx2_select_i32_64((u8*)i32any_ptr(w), (u8*)f64any_ptr(x), (u8*)rp, wia, xia)) thrM("⊏: Indexing out-of-bounds"); goto dec_ret; }
    #endif
    #define CASE(E,TY) { \
      E* rp = m_tyarrc(&r, sizeof(E), w, TY); \
      E* xp = tyany_ptr(x);                   \
      for (usz i = 0; i < wia; i++) rp[i] = xp[WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia))]; \
      goto dec_ret;                           \
    }
    #define TYPE(W) { W* wp = W##any_ptr(w);  \
      switch(xe) { default: UD;               \
        case el_bit: { u64* xp=bitarr_ptr(x); \
          u64* rp; r = m_bitarrc(&rp, w);   \
          for (usz i = 0; i < wia; i++) bitp_set(rp, i, bitp_get(xp, WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia)))); \
          goto dec_ret;                       \
        }                                     \
        case el_i8: case el_c8: CASE(u8 ,el2t(xe)) \
        case el_i16:case el_c16:CASE(u16,el2t(xe)) \
        case el_i32:case el_c32:CASE(u32,el2t(xe)) \
        case el_f64:            CASE(f64,t_f64arr) \
        case el_B:; \
      }              \
      M_HARR(r, wia); \
      if (TY(x)==t_harr || TY(x)==t_hslice) {     \
        B* xp = hany_ptr(x);                      \
        for (usz i=0; i < wia; i++) HARR_ADD(r, i, inc(xp[WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia))])); \
        decG(x); return HARR_FCD(r, w);           \
      } SLOW2("𝕨⊏𝕩", w, x);                       \
      for (usz i=0; i < wia; i++) HARR_ADD(r, i, Get(x, WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia)))); \
      decG(x); return withFill(HARR_FCD(r,w),xf); \
    }
    if (we==el_bit && xia>=2) {
      SGetU(x)
      r = bit_sel(w, GetU(x,0), true, GetU(x,1), true);
      decG(x);
      return withFill(r, xf);
    }
    else if (we==el_i8) TYPE(i8)
    else if (we==el_i16) TYPE(i16)
    else if (we==el_i32) TYPE(i32)
    else {
      SLOW2("𝕨⊏𝕩", w, x);
      M_HARR(r, wia)
      SGetU(w)
      for (usz i = 0; i < wia; i++) {
        B cw = GetU(w, i);
        if (!isNum(cw)) { HARR_ABANDON(r); goto base; }
        usz c = WRAP(o2i64(cw), xia, thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", cw, xia));
        HARR_ADD(r, i, Get(x, c));
      }
      decG(x);
      return withFill(HARR_FCD(r,w),xf);
    }
    #undef CASE
  } else {
    SLOW2("𝕨⊏𝕩", w, x);
    SGetU(w)
    ur wr = RNK(w);
    i32 rr = wr+xr-1;
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    if (rr>UR_MAX) thrF("⊏: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
    usz csz = arr_csz(x);
    usz cam = SH(x)[0];
    MAKE_MUT(r, wia*csz); mut_init(r, TI(x,elType));
    MUTG_INIT(r);
    for (usz i = 0; i < wia; i++) {
      B cw = GetU(w, i);
      if (!isNum(cw)) { mut_pfree(r, i*csz); goto base; }
      f64 c = o2f(cw);
      if (c<0) c+= cam;
      if ((usz)c >= cam) { mut_pfree(r, i*csz); thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", cw, cam); }
      mut_copyG(r, i*csz, x, csz*(usz)c, csz);
    }
    Arr* ra = mut_fp(r);
    usz* rsh = arr_shAlloc(ra, rr);
    if (rsh) {
      shcpy(rsh   , SH(w)  , wr  );
      shcpy(rsh+wr, SH(x)+1, xr-1);
    }
    decG(w); decG(x);
    return withFill(taga(ra),xf);
  }
  base:;
  dec(xf);
  return c2(rt_select, w, x);
  
  dec_ret:;
  decG(w); decG(x); return r;
}




B select_ucw(B t, B o, B w, B x) {
  if (isAtm(x) || RNK(x)!=1 || isAtm(w)) return def_fn_ucw(t, o, w, x);
  usz xia = IA(x);
  usz wia = IA(w);
  SGetU(w)
  if (TI(w,elType)!=el_i32) for (usz i = 0; i < wia; i++) if (!q_i64(GetU(w,i))) return def_fn_ucw(t, o, w, x);
  B arg = select_c2(t, inc(w), inc(x));
  B rep = c1(o, arg);
  if (isAtm(rep) || !eqShape(w, rep)) thrF("𝔽⌾(a⊸⊏)𝕩: Result of 𝔽 must have the same shape as 'a' (expected %H, got %H)", w, rep);
  #if CHECK_VALID
    TALLOC(bool, set, xia);
    for (i64 i = 0; i < xia; i++) set[i] = false;
    #define EQ(F) if (set[cw] && (F)) thrM("𝔽⌾(a⊸⊏): Incompatible result elements"); set[cw] = true;
    #define FREE_CHECK TFREE(set)
    SLOWIF(xia>100 && wia<xia/20) SLOW2("⌾(𝕨⊸⊏)𝕩 because CHECK_VALID", w, x);
  #else
    #define EQ(F)
    #define FREE_CHECK
  #endif
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  u8 re = TI(rep,elType);
  SLOWIF(!reusable(x) && xia>100 && wia<xia/50) SLOW2("⌾(𝕨⊸⊏)𝕩 because not reusable", w, x);
  if (elInt(we)) {
    w = toI32Any(w);
    i32* wp = i32any_ptr(w);
    if (elNum(re) && elNum(xe)) {
      u8 me = xe>re?xe:re;
      bool reuse = reusable(x);
      if (me==el_i32) {
        I32Arr* xn = reuse? toI32Arr(REUSE(x)) : cpyI32Arr(x);
        i32* xp = i32arrv_ptr(xn);
        rep = toI32Any(rep); i32* rp = i32any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia; // we're free to assume w is valid
          i32 cr = rp[i];
          EQ(cr != xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_i8) {
        I8Arr* xn = reuse? toI8Arr(REUSE(x)) : cpyI8Arr(x);
        i8* xp = i8arrv_ptr(xn);
        rep = toI8Any(rep); i8* rp = i8any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          i8 cr = rp[i];
          EQ(cr != xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_i16) {
        I16Arr* xn = reuse? toI16Arr(REUSE(x)) : cpyI16Arr(x);
        i16* xp = i16arrv_ptr(xn);
        rep = toI16Any(rep); i16* rp = i16any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          i16 cr = rp[i];
          EQ(cr != xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_bit) {
        BitArr* xn = reuse? toBitArr(REUSE(x)) : cpyBitArr(x);
        u64* xp = bitarrv_ptr(xn);
        rep = taga(toBitArr(rep)); u64* rp = bitarr_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          bool cr = bitp_get(rp, i);
          EQ(cr != bitp_get(xp,cw));
          bitp_set(xp,cw,cr);
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_f64) {
        F64Arr* xn = reuse? toF64Arr(REUSE(x)) : cpyF64Arr(x);
        f64* xp = f64arrv_ptr(xn);
        rep = toF64Any(rep); f64* rp = f64any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          f64 cr = rp[i];
          EQ(cr != xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else UD;
    }
    if (reusable(x) && xe==re) {
      if (TY(x)==t_harr) {
        B* xp = harr_ptr(REUSE(x));
        SGet(rep)
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          B cr = Get(rep, i);
          EQ(!equal(cr,xp[cw]));
          dec(xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK;
        return x;
      }
    }
    MAKE_MUT(r, xia); mut_init(r, el_or(xe, re));
    MUTG_INIT(r);
    mut_copyG(r, 0, x, 0, xia);
    SGet(rep)
    for (usz i = 0; i < wia; i++) {
      i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
      B cr = Get(rep, i);
      EQ(!equal(mut_getU(r, cw), cr));
      mut_rm(r, cw);
      mut_setG(r, cw, cr);
    }
    decG(w); decG(rep); FREE_CHECK;
    return mut_fcd(r, x);
  }
  MAKE_MUT(r, xia); mut_init(r, el_or(xe, re));
  MUTG_INIT(r);
  mut_copyG(r, 0, x, 0, xia);
  SGet(rep)
  for (usz i = 0; i < wia; i++) {
    i64 cw = o2i64u(GetU(w, i)); if (RARE(cw<0)) cw+= (i64)xia;
    B cr = Get(rep, i);
    EQ(!equal(mut_getU(r, cw), cr));
    mut_rm(r, cw);
    mut_setG(r, cw, cr);
  }
  decG(w); decG(rep); FREE_CHECK;
  return mut_fcd(r, x);
  #undef EQ
  #undef FREE_CHECK
}
