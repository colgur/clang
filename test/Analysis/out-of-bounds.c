// RUN: %clang_cc1 -analyze -analyzer-check-objc-mem -analyzer-check-buffer-overflows -verify %s

// Tests doing an out-of-bounds access after the end of an array using:
// - constant integer index
// - constant integer size for buffer
void test1(int x) {
  int buf[100];
  buf[100] = 1; // expected-warning{{Out of bound memory access}}
}

void test1_ok(int x) {
  int buf[100];
  buf[99] = 1; // no-warning
}

const char test1_strings_underrun(int x) {
  const char *mystr = "mary had a little lamb";
  return mystr[-1]; // expected-warning{{Out of bound memory access}}
}

const char test1_strings_overrun(int x) {
  const char *mystr = "mary had a little lamb";
  return mystr[1000];  // expected-warning{{Out of bound memory access}}
}

const char test1_strings_ok(int x) {
  const char *mystr = "mary had a little lamb";
  return mystr[5]; // no-warning
}

// Tests doing an out-of-bounds access after the end of an array using:
// - indirect pointer to buffer
// - constant integer index
// - constant integer size for buffer
void test1_ptr(int x) {
  int buf[100];
  int *p = buf;
  p[101] = 1; // expected-warning{{Out of bound memory access}}
}

void test1_ptr_ok(int x) {
  int buf[100];
  int *p = buf;
  p[99] = 1; // no-warning
}

// Tests doing an out-of-bounds access before the start of an array using:
// - indirect pointer to buffer, manipulated using simple pointer arithmetic
// - constant integer index
// - constant integer size for buffer
void test1_ptr_arith(int x) {
  int buf[100];
  int *p = buf;
  p = p + 100;
  p[0] = 1; // expected-warning{{Out of bound memory access}}
}

void test1_ptr_arith_ok(int x) {
  int buf[100];
  int *p = buf;
  p = p + 99;
  p[0] = 1; // no-warning
}

void test1_ptr_arith_bad(int x) {
  int buf[100];
  int *p = buf;
  p = p + 99;
  p[1] = 1; // expected-warning{{Out of bound memory access}}
}

void test1_ptr_arith_ok2(int x) {
  int buf[100];
  int *p = buf;
  p = p + 99;
  p[-1] = 1; // no-warning
}

// Tests doing an out-of-bounds access before the start of an array using:
// - constant integer index
// - constant integer size for buffer
void test2(int x) {
  int buf[100];
  buf[-1] = 1; // expected-warning{{Out of bound memory access}}
}

// Tests doing an out-of-bounds access before the start of an array using:
// - indirect pointer to buffer
// - constant integer index
// - constant integer size for buffer
void test2_ptr(int x) {
  int buf[100];
  int *p = buf;
  p[-1] = 1; // expected-warning{{Out of bound memory access}}
}

// ** FIXME ** Doesn't work yet because we don't support pointer arithmetic.
// Tests doing an out-of-bounds access before the start of an array using:
// - indirect pointer to buffer, manipulated using simple pointer arithmetic
// - constant integer index
// - constant integer size for buffer
void test2_ptr_arith(int x) {
  int buf[100];
  int *p = buf;
  --p;
  p[0] = 1; // no-warning
}

// Tests doing an out-of-bounds access before the start of a multi-dimensional
// array using:
// - constant integer indices
// - constant integer sizes for the array
void test2_multi(int x) {
  int buf[100][100];
  buf[0][-1] = 1; // expected-warning{{Out of bound memory access}}
}

// Tests doing an out-of-bounds access before the start of a multi-dimensional
// array using:
// - constant integer indices
// - constant integer sizes for the array
void test2_multi_b(int x) {
  int buf[100][100];
  buf[-1][0] = 1; // expected-warning{{Out of bound memory access}}
}

void test2_multi_ok(int x) {
  int buf[100][100];
  buf[0][0] = 1; // no-warning
}

// *** FIXME ***
// We don't get a warning here yet because our symbolic constraint solving
// doesn't handle:  (symbol * constant) < constant
void test3(int x) {
  int buf[100];
  if (x < 0)
    buf[x] = 1; 
}

// *** FIXME ***
// We don't get a warning here yet because our symbolic constraint solving
// doesn't handle:  (symbol * constant) < constant
void test4(int x) {
  int buf[100];
  if (x > 99)
    buf[x] = 1; 
}
