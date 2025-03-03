typedef struct FillArr {
  struct Arr;
  B fill;
  B a[];
} FillArr;
typedef struct FillSlice {
  struct Slice;
  B* a;
  B fill;
} FillSlice;

B asFill(B x); // consumes
void validateFill(B x);

B withFill(B x, B fill); // consumes both
static B qWithFill(B x, B fill) { // consumes both
  assert(isArr(x));
  if (noFill(fill) || TI(x,elType)!=el_B) return x;
  return withFill(x, fill);
}

NOINLINE bool fillEqualF(B w, B x);
static bool fillEqual(B w, B x) {
  if (w.u==x.u) return true;
  if (isAtm(w)|isAtm(x)) return false;
  return fillEqualF(w, x);
}


static B getFillR(B x) { // doesn't consume; can return bi_noFill
  if (isArr(x)) {
    switch(TI(x,elType)) { default: UD;
      case el_i8: case el_i16: case el_i32: case el_f64: case el_bit: return m_i32(0);
      case el_c8: case el_c16: case el_c32: return m_c32(' ');
      case el_B:;
        u8 t = TY(x);
        if (t==t_fillarr  ) return inc(c(FillArr,  x)->fill);
        if (t==t_fillslice) return inc(c(FillSlice,x)->fill);
        return bi_noFill;
    }
  }
  if (isNum(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  return bi_noFill;
}
static B getFillQ(B x) { // doesn't consume; returns 0 if !CATCH_ERRORS
  B r = getFillR(x);
  #if CATCH_ERRORS
    return r;
  #endif
  return noFill(r)? m_f64(0) : r;
}
static B getFillE(B x) { // errors if there's no fill
  B xf = getFillQ(x);
  if (noFill(xf)) {
    if (PROPER_FILLS) thrM("No fill found");
    else return m_f64(0);
  }
  return xf;
}


static Arr* m_fillarrp(usz ia) {
  CHECK_IA(ia, sizeof(B));
  return m_arr(fsizeof(FillArr,a,B,ia), t_fillarr, ia);
}
static void fillarr_setFill(Arr* x, B fill) { assert(PTY(x)==t_fillarr); ((FillArr*)x)->fill = fill; } // consumes fill
static B* fillarr_ptr(Arr* x) { assert(PTY(x)==t_fillarr); return ((FillArr*)x)->a; }

static B m_emptyFVec(B f) { // consumes f
  Arr* r = arr_shVec(m_fillarrp(0));
  fillarr_setFill(r, f);
  return taga(r);
}

B m_unit(B x); // consumes
B m_atomUnit(B x); // consumes

static B fill_or(B wf, B xf) { // consumes
  if (fillEqual(wf, xf)) {
    dec(wf);
    return xf;
  }
  dec(wf); dec(xf);
  return bi_noFill;
}

static B fill_both(B w, B x) { // doesn't consume
  B wf = getFillQ(w);
  if (noFill(wf)) return bi_noFill;
  B xf = getFillQ(x);
  return fill_or(wf, xf);
}
