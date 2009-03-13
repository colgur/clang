// RUN: clang -fsyntax-only -verify %s

template<int I, int J>
struct Bitfields {
  int simple : I; // expected-error{{bit-field 'simple' has zero width}}
  int parens : (J);
};

void test_Bitfields(Bitfields<0, 5> *b) {
  (void)sizeof(Bitfields<10, 5>);
  (void)sizeof(Bitfields<0, 1>); // expected-note{{in instantiation of template class 'struct Bitfields<0, 1>' requested here}}
}

template<int I, int J>
struct BitfieldPlus {
  int bitfield : I + J; // expected-error{{bit-field 'bitfield' has zero width}}
};

void test_BitfieldPlus() {
  (void)sizeof(BitfieldPlus<0, 1>);
  (void)sizeof(BitfieldPlus<-5, 5>); // expected-note{{in instantiation of template class 'struct BitfieldPlus<-5, 5>' requested here}}
}

template<int I, int J>
struct BitfieldMinus {
  int bitfield : I - J; // expected-error{{bit-field 'bitfield' has negative width (-1)}} \
  // expected-error{{bit-field 'bitfield' has zero width}}
};

void test_BitfieldMinus() {
  (void)sizeof(BitfieldMinus<5, 1>);
  (void)sizeof(BitfieldMinus<0, 1>); // expected-note{{in instantiation of template class 'struct BitfieldMinus<0, 1>' requested here}}
  (void)sizeof(BitfieldMinus<5, 5>); // expected-note{{in instantiation of template class 'struct BitfieldMinus<5, 5>' requested here}}
}

template<int I, int J>
struct BitfieldDivide {
  int bitfield : I / J; // expected-error{{expression is not an integer constant expression}} \
                        // expected-note{{division by zero}}
};

void test_BitfieldDivide() {
  (void)sizeof(BitfieldDivide<5, 1>);
  (void)sizeof(BitfieldDivide<5, 0>); // expected-note{{in instantiation of template class 'struct BitfieldDivide<5, 0>' requested here}}
}

template<typename T, T I, int J>
struct BitfieldDep {
  int bitfield : I + J;
};

void test_BitfieldDep() {
  (void)sizeof(BitfieldDep<int, 1, 5>);
}

