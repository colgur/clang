// RUN: %clang_cc1 -fsyntax-only -verify %s

// C++'0x [class.friend] p1:
//   A friend of a class is a function or class that is given permission to use
//   the private and protected member names from the class. A class specifies
//   its friends, if any, by way of friend declarations. Such declarations give
//   special access rights to the friends, but they do not make the nominated
//   friends members of the befriending class.

struct S { static void f(); };
S* g() { return 0; }

struct X {
  friend struct S;
  friend S* g();
};

void test1() {
  S s;
  g()->f();
  S::f();
  X::g(); // expected-error{{no member named 'g' in 'X'}}
  X::S x_s; // expected-error{{no member named 'S' in 'X'}}
  X x;
  x.g(); // expected-error{{no member named 'g' in 'X'}}
}

// Test that we recurse through namespaces to find already declared names, but
// new names are declared within the enclosing namespace.
namespace N {
  struct X {
    friend struct S;
    friend S* g();

    friend struct S2;
    friend struct S2* g2();
  };

  struct S2 { static void f2(); };
  S2* g2() { return 0; }

  void test() {
    g()->f();
    S s;
    S::f();
    X::g(); // expected-error{{no member named 'g' in 'N::X'}}
    X::S x_s; // expected-error{{no member named 'S' in 'N::X'}}
    X x;
    x.g(); // expected-error{{no member named 'g' in 'N::X'}}

    g2();
    S2 s2;
    ::g2(); // expected-error{{no member named 'g2' in the global namespace}}
    ::S2 g_s2; // expected-error{{no member named 'S2' in the global namespace}}
    X::g2(); // expected-error{{no member named 'g2' in 'N::X'}}
    X::S2 x_s2; // expected-error{{no member named 'S2' in 'N::X'}}
    x.g2(); // expected-error{{no member named 'g2' in 'N::X'}}
  }
}

namespace test0 {
  class ClassFriend {
    void test();
  };

  class MemberFriend {
    void test();
  };

  void declared_test();

  class Class {
    static void member(); // expected-note 2 {{declared private here}}

    friend class ClassFriend;
    friend class UndeclaredClassFriend;

    friend void undeclared_test();
    friend void declared_test();
    friend void MemberFriend::test();
  };

  void declared_test() {
    Class::member();
  }

  void undeclared_test() {
    Class::member();
  }

  void unfriended_test() {
    Class::member(); // expected-error {{'member' is a private member of 'test0::Class'}}
  }

  void ClassFriend::test() {
    Class::member();
  }

  void MemberFriend::test() {
    Class::member();
  }

  class UndeclaredClassFriend {
    void test() {
      Class::member();
    }
  };

  class ClassNonFriend {
    void test() {
      Class::member(); // expected-error {{'member' is a private member of 'test0::Class'}}
    }
  };
}

// Make sure that friends have access to inherited protected members.
namespace test2 {
  struct X;

  class ilist_half_node {
    friend struct ilist_walker_bad;
    X *Prev;
  protected:
    X *getPrev() { return Prev; } // expected-note{{member is declared here}}
  };

  class ilist_node : private ilist_half_node { // expected-note {{declared private here}} expected-note {{constrained by private inheritance here}}
    friend struct ilist_walker;
    X *Next;
    X *getNext() { return Next; } // expected-note {{declared private here}}
  };

  struct X : ilist_node {};

  struct ilist_walker {
    static X *getPrev(X *N) { return N->getPrev(); }
    static X *getNext(X *N) { return N->getNext(); }
  };  

  struct ilist_walker_bad {
    static X *getPrev(X *N) { return N->getPrev(); } // \
    // expected-error {{'getPrev' is a private member of 'test2::ilist_half_node'}} \
    // expected-error {{cannot cast 'test2::X' to its private base class 'test2::ilist_half_node'}}

    static X *getNext(X *N) { return N->getNext(); } // \
    // expected-error {{'getNext' is a private member of 'test2::ilist_node'}}
  };  
}

namespace test3 {
  class A { protected: int x; }; // expected-note {{declared protected here}}

  class B : public A {
    friend int foo(B*);
  };

  int foo(B *p) {
    return p->x;
  }

  int foo(const B *p) {
    return p->x; // expected-error {{'x' is a protected member of 'test3::A'}}
  }
}

namespace test3a {
  class A { protected: int x; };

  class B : public A {
    friend int foo(B*);
  };

  int foo(B * const p) {
    return p->x;
  }
}

namespace test4 {
  template <class T> class Holder {
    T object;
    friend bool operator==(Holder &a, Holder &b) {
      return a.object == b.object; // expected-error {{invalid operands to binary expression}}
    }
  };

  struct Inequal {};
  bool test() {
    Holder<Inequal> a, b;
    return a == b;  // expected-note {{requested here}}
  }
}


// PR6174
namespace test5 {
  namespace ns {
    class A;
  }

  class ns::A {
  private: int x;
    friend class B;
  };

  namespace ns {
    class B {
      int test(A *p) { return p->x; }
    };
  }
}

// PR6207
namespace test6 {
  struct A {};

  struct B {
    friend A::A();
    friend A::~A();
    friend A &A::operator=(const A&);
  };
}

namespace test7 {
  template <class T> struct X {
    X();
    ~X();
    void foo();
    void bar();
  };

  class A {
    friend void X<int>::foo();
    friend X<int>::X();
    friend X<int>::X(const X&);

  private:
    A(); // expected-note 2 {{declared private here}}
  };

  template<> void X<int>::foo() {
    A a;
  }

  template<> void X<int>::bar() {
    A a; // expected-error {{calling a private constructor}}
  }

  template<> X<int>::X() {
    A a;
  }

  template<> X<int>::~X() {
    A a; // expected-error {{calling a private constructor}}
  }
}

// Return types, parameters and default arguments to friend functions.
namespace test8 {
  class A {
    typedef int I; // expected-note 4 {{declared private here}}
    static const I x = 0; // expected-note {{implicitly declared private here}}
    friend I f(I i);
    template<typename T> friend I g(I i);
  };

  const A::I A::x;
  A::I f(A::I i = A::x) {}
  template<typename T> A::I g(A::I i) {
    T t;
  }
  template A::I g<A::I>(A::I i);

  A::I f2(A::I i = A::x) {} // expected-error 3 {{is a private member of}}
  template<typename T> A::I g2(A::I i) { // expected-error 2 {{is a private member of}}
    T t;
  }
  template A::I g2<A::I>(A::I i);
}

// PR6885
namespace test9 {
  class B {
    friend class test9;
  };
}

// PR7230
namespace test10 {
  extern "C" void f(void);
  extern "C" void g(void);

  namespace NS {
    class C {
      void foo(void); // expected-note {{declared private here}}
      friend void test10::f(void);
    };
    static C* bar;
  }

  void f(void) {
    NS::bar->foo();
  }

  void g(void) {
    NS::bar->foo(); // expected-error {{private member}}
  }
}
