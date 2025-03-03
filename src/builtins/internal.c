#include "../core.h"
#include "../utils/mut.h"
#include "../utils/file.h"
#include "../builtins.h"
#include "../ns.h"

B itype_c1(B t, B x) {
  B r;
  if(isVal(x)) {
    r = m_c8vec_0(type_repr(TY(x)));
  } else {
    if      (isF64(x)) r = m_c8vec_0("tagged f64");
    else if (isC32(x)) r = m_c8vec_0("tagged c32");
    else if (isTag(x)) r = m_c8vec_0("tagged tag");
    else if (isVar(x)) r = m_c8vec_0("tagged var");
    else if (isExt(x)) r = m_c8vec_0("tagged extvar");
    else               r = m_c8vec_0("tagged unknown");
  }
  dec(x);
  return r;
}
B elType_c1(B t, B x) {
  B r = m_i32(isArr(x)? TI(x,elType) : selfElType(x));
  dec(x);
  return r;
}
B refc_c1(B t, B x) {
  B r = isVal(x)? m_i32(v(x)->refc) : m_c8vec_0("(not heap-allocated)");
  dec(x);
  return r;
}
B squeeze_c1(B t, B x) {
  if (!isArr(x)) return x;
  return any_squeeze(x);
}
B deepSqueeze_c1(B t, B x) {
  return squeeze_deep(x);
}
B isPure_c1(B t, B x) {
  B r = m_f64(isPureFn(x));
  dec(x);
  return r;
}

B info_c2(B t, B w, B x) {
  B s = emptyCVec();
  i32 m = o2i(w);
  if (isVal(x)) {
    if (m) AFMT("%xl: ", x.u);
    else AFMT("%xl: ", x.u>>48);
    Value* xv = v(x);
    AFMT("refc:%i ", xv->refc);
    if (m) {
      AFMT("mmInfo:%i ", xv->mmInfo);
      AFMT("flags:%i ", xv->flags);
      AFMT("extra:%i ", xv->extra);
    }
    AFMT("type:%i=%S ", PTY(xv), type_repr(PTY(xv)));
    AFMT("alloc:%l", mm_size(xv));
    decG(x);
  } else {
    AFMT("%xl: ", x.u);
    A8("not heap-allocated");
  }
  return s;
}
B info_c1(B t, B x) {
  return info_c2(t, m_i32(0), x);
}

#define FOR_VARIATION(F) F(Ai8 ) F(Si8 ) F(Ai8Inc ) F(Si8Inc ) \
                         F(Ai16) F(Si16) F(Ai16Inc) F(Si16Inc) \
                         F(Ai32) F(Si32) F(Ai32Inc) F(Si32Inc) \
                         F(Ac8 ) F(Sc8 ) F(Ac8Inc ) F(Sc8Inc ) \
                         F(Ac16) F(Sc16) F(Ac16Inc) F(Sc16Inc) \
                         F(Ac32) F(Sc32) F(Ac32Inc) F(Sc32Inc) \
                         F(Af64) F(Sf64) F(Af64Inc) F(Sf64Inc) \
                         F(Ah)   F(Sh)   F(AhInc)   F(ShInc) \
                         F(Af)   F(Sf)   F(AfInc)   F(SfInc) \
                         F(Ab)           F(AbInc)

#define F(X) static B v_##X;
FOR_VARIATION(F)
#undef F
static B listVariations_def;

B listVariations_c2(B t, B w, B x) {
  if (!isArr(x)) thrM("•internal.ListVariations: 𝕩 must be an array");
  
  if (!isArr(w) || RNK(w)!=1) thrM("•internal.ListVariations: 𝕨 must be a list");
  usz wia = IA(w);
  SGetU(w)
  bool c_incr=false, c_rmFill=false;
  for (usz i = 0; i < wia; i++) {
    u32 c = o2c(GetU(w, i));
    if (c=='i') c_incr=true;
    else if (c=='f') c_rmFill=true;
    else thrF("internal.ListVariations: Unknown option '%c' in 𝕨", c);
  }
  decG(w);
  
  u8 xe = TI(x,elType);
  B xf = getFillQ(x);
  bool ah = c_rmFill || noFill(xf);
  bool ai8=false, ai16=false, ai32=false, af64=false,
       ac8=false, ac16=false, ac32=false, abit=false;
  usz xia = IA(x);
  SGetU(x)
  if (isNum(xf)) {
    i32 min=I32_MAX, max=I32_MIN;
    if      (xe==el_i8 ) { i8*  xp = i8any_ptr (x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_i16) { i16* xp = i16any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_i32) { i32* xp = i32any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_f64) { f64* xp = f64any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; if(xp[i]!=(i32)xp[i]) goto onlyF64; } }
    else for (usz i = 0; i < xia; i++) { B c = GetU(x, i); if (!isF64(c)) goto noSpec; if (c.f>max) max=c.f; if (c.f<min) min=c.f; }
    ai8  = min==(i8 )min && max==(i8 )max;
    ai16 = min==(i16)min && max==(i16)max;
    ai32 = min==(i32)min && max==(i32)max;
    abit = min>=0 && max<=1;
    onlyF64:
    af64 = true;
  } else if (isC32(xf)) {
    u32 max = 0;
    if (xe!=el_c8) for (usz i = 0; i < xia; i++) {
      B c = GetU(x, i);
      if (!isC32(c)) goto noSpec;
      if (o2cu(c)>max) max = o2cu(c);
    }
    ac8  = max == (u8 )max;
    ac16 = max == (u16)max;
    ac32 = true;
  }
  noSpec:;
  B r = emptyHVec();
  if(abit) { r=vec_addN(r,incG(v_Ab  ));                            if(c_incr) { r=vec_addN(r,incG(v_AbInc  ));                               } }
  if(ai8 ) { r=vec_addN(r,incG(v_Ai8 ));r=vec_addN(r,incG(v_Si8 )); if(c_incr) { r=vec_addN(r,incG(v_Ai8Inc ));r=vec_addN(r,incG(v_Si8Inc )); } }
  if(ai16) { r=vec_addN(r,incG(v_Ai16));r=vec_addN(r,incG(v_Si16)); if(c_incr) { r=vec_addN(r,incG(v_Ai16Inc));r=vec_addN(r,incG(v_Si16Inc)); } }
  if(ai32) { r=vec_addN(r,incG(v_Ai32));r=vec_addN(r,incG(v_Si32)); if(c_incr) { r=vec_addN(r,incG(v_Ai32Inc));r=vec_addN(r,incG(v_Si32Inc)); } }
  if(ac8 ) { r=vec_addN(r,incG(v_Ac8 ));r=vec_addN(r,incG(v_Sc8 )); if(c_incr) { r=vec_addN(r,incG(v_Ac8Inc ));r=vec_addN(r,incG(v_Sc8Inc )); } }
  if(ac16) { r=vec_addN(r,incG(v_Ac16));r=vec_addN(r,incG(v_Sc16)); if(c_incr) { r=vec_addN(r,incG(v_Ac16Inc));r=vec_addN(r,incG(v_Sc16Inc)); } }
  if(ac32) { r=vec_addN(r,incG(v_Ac32));r=vec_addN(r,incG(v_Sc32)); if(c_incr) { r=vec_addN(r,incG(v_Ac32Inc));r=vec_addN(r,incG(v_Sc32Inc)); } }
  if(af64) { r=vec_addN(r,incG(v_Af64));r=vec_addN(r,incG(v_Sf64)); if(c_incr) { r=vec_addN(r,incG(v_Af64Inc));r=vec_addN(r,incG(v_Sf64Inc)); } }
  if(ah)   { r=vec_addN(r,incG(v_Ah  ));r=vec_addN(r,incG(v_Sh  )); if(c_incr) { r=vec_addN(r,incG(v_AhInc  ));r=vec_addN(r,incG(v_ShInc  )); } }
  {          r=vec_addN(r,incG(v_Af  ));r=vec_addN(r,incG(v_Sf  )); if(c_incr) { r=vec_addN(r,incG(v_AfInc  ));r=vec_addN(r,incG(v_SfInc  )); } }
  decG(x);
  dec(xf);
  return r;
}
B listVariations_c1(B t, B x) {
  return listVariations_c2(t, incG(listVariations_def), x);
}
static bool u8_get(u8** cv, u8* cE, const char* x) {
  u8* c = *cv;
  while (true) {
    if (!*x) {
      *cv = c;
      return true;
    }
    if (c==cE || *c!=*x) return false;
    c++; x++;
  }
  
}

static B variation_refs;
static void variation_gcRoot() {
  mm_visit(variation_refs);
  mm_visit(listVariations_def);
  #define F(X) mm_visit(v_##X);
  FOR_VARIATION(F)
  #undef F
}

B variation_c2(B t, B w, B x) {
  if (!isArr(w)) thrM("•internal.Variation: Non-array 𝕨");
  if (!isArr(x)) thrM("•internal.Variation: Non-array 𝕩");
  usz xia = IA(x);
  C8Arr* wc = toC8Arr(w);
  u8* wp = c8arrv_ptr(wc);
  u8* wpE = wp+PIA(wc);
  if (PIA(wc)==0) thrM("•internal.Variation: Zero-length 𝕨");
  B res;
  if (*wp == 'A' || *wp == 'S') {
    bool slice = *wp == 'S';
    wp++;
    if      (u8_get(&wp, wpE, "b"  )) res = taga(cpyBitArr(incG(x)));
    else if (u8_get(&wp, wpE, "i8" )) res = taga(cpyI8Arr (incG(x)));
    else if (u8_get(&wp, wpE, "i16")) res = taga(cpyI16Arr(incG(x)));
    else if (u8_get(&wp, wpE, "i32")) res = taga(cpyI32Arr(incG(x)));
    else if (u8_get(&wp, wpE, "c8" )) res = taga(cpyC8Arr (incG(x)));
    else if (u8_get(&wp, wpE, "c16")) res = taga(cpyC16Arr(incG(x)));
    else if (u8_get(&wp, wpE, "c32")) res = taga(cpyC32Arr(incG(x)));
    else if (u8_get(&wp, wpE, "f64")) res = taga(cpyF64Arr(incG(x)));
    else if (u8_get(&wp, wpE, "h"  )) res = taga(cpyHArr  (incG(x)));
    else if (u8_get(&wp, wpE, "f")) {
      Arr* t = m_fillarrp(xia);
      fillarr_setFill(t, getFillQ(x));
      arr_shCopy(t, x);
      HArr* h = NULL;
      
      B* xp = arr_bptr(x);
      if (xp==NULL) {
        h = cpyHArr(incG(x));
        xp = h->a;
      }
      
      B* rp = fillarr_ptr(t);
      for (usz i = 0; i < xia; i++) rp[i] = inc(xp[i]);
      if (h) ptr_dec(h);
      
      res = taga(t);
    } else thrF("•internal.Variation: Bad type \"%R\"", taga(wc));
    
    if (slice) {
      Arr* slice = TI(res,slice)(inc(res), 0, IA(res));
      arr_shCopy(slice, res);
      dec(res);
      res = taga(slice);
    }
    
    if (u8_get(&wp, wpE, "Inc")) {
      if (!variation_refs.u) {
        variation_refs = emptyHVec();
      }
      variation_refs = vec_addN(variation_refs, incG(res));
    }
    if (wp!=wpE) thrM("•internal.Variation: Bad 𝕨");
  } else thrM("•internal.Variation: Bad start of 𝕨");
  decG(x);
  ptr_dec(wc);
  return res;
}

B clearRefs_c1(B t, B x) {
  dec(x);
  if (!isArr(variation_refs)) return m_f64(0);
  usz res = IA(variation_refs);
  dec(variation_refs);
  variation_refs = m_f64(0);
  return m_f64(res);
}

static B unshare(B x) {
  if (!isArr(x)) return x;
  usz xia = IA(x);
  switch (TY(x)) {
    case t_bitarr: return taga(cpyBitArr(inc(x)));
    case t_i8arr:  return taga(cpyI8Arr (inc(x)));
    case t_i16arr: return taga(cpyI16Arr(inc(x)));
    case t_i32arr: return taga(cpyI32Arr(inc(x)));
    case t_c8arr:  return taga(cpyC8Arr (inc(x)));
    case t_c16arr: return taga(cpyC16Arr(inc(x)));
    case t_c32arr: return taga(cpyC32Arr(inc(x)));
    case t_f64arr: return taga(cpyF64Arr(inc(x)));
    case t_harr: {
      B* xp = harr_ptr(x);
      M_HARR(r, xia)
      for (usz i = 0; i < xia; i++) HARR_ADD(r, i, unshare(xp[i]));
      return HARR_FC(r, x);
    }
    case t_fillarr: {
      Arr* r = m_fillarrp(xia); arr_shCopy(r, x);
      fillarr_setFill(r, unshare(c(FillArr,x)->fill));
      B* rp = fillarr_ptr(r); B* xp = fillarr_ptr(a(x));
      for (usz i = 0; i < xia; i++) rp[i] = unshare(xp[i]);
      return taga(r);
    }
    default: thrF("•internal.Unshare: Cannot unshare array with type %i=%S", TY(x), type_repr(TY(x)));
  }
}



B eequal_c2(B t, B w, B x) {
  bool r = eequal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}
      

B internalTemp_c1(B t, B x) {
  #ifdef TEST_BITCPY
  SGetU(x)
  bit_cpy(bitarr_ptr(GetU(x,0)), o2s(GetU(x,1)), bitarr_ptr(GetU(x,2)), o2s(GetU(x,3)), o2s(GetU(x,4)));
  #endif
  return x;
}

B internalTemp_c2(B t, B w, B x) { dec(w); return x; }

B heapDump_c1(B t, B x) {
  cbqn_heapDump();
  return m_c32(0);
}

B unshare_c1(B t, B x) {
  if (!isArr(x)) thrM("•internal.Unshare: Argument must be an array");
  B r = unshare(x);
  dec(x);
  return r;
}
static B internalNS;
B getInternalNS() {
  if (internalNS.u == 0) {
    #define F(X) v_##X = m_c8vec_0(#X);
    FOR_VARIATION(F)
    #undef F
    listVariations_def = m_c8vec_0("if");
    gc_addFn(variation_gcRoot);
    #define F(X) incG(bi_##X),
    Body* d =    m_nnsDesc("type","eltype","refc","squeeze","ispure","info","listvariations","variation","clearrefs","unshare","deepsqueeze","heapdump","eequal","temp");
    internalNS = m_nns(d,F(itype)F(elType)F(refc)F(squeeze)F(isPure)F(info)F(listVariations)F(variation)F(clearRefs)F(unshare)F(deepSqueeze)F(heapDump)F(eequal)F(internalTemp));
    #undef F
    gc_add(internalNS);
  }
  return incG(internalNS);
}
