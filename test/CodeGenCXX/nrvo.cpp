// RUN: %clang_cc1 -emit-llvm -O1 -o - %s | FileCheck %s
// RUN: %clang_cc1 -emit-llvm -O1 -fexceptions -o - %s | FileCheck --check-prefix=CHECK-EH %s

// Test code generation for the named return value optimization.
class X {
public:
  X();
  X(const X&);
  ~X();
};

// CHECK: define void @_Z5test0v
// CHECK-EH: define void @_Z5test0v
X test0() {
  X x;
  // CHECK:          call void @_ZN1XC1Ev
  // CHECK-NEXT:     ret void

  // CHECK-EH:       call void @_ZN1XC1Ev
  // CHECK-EH-NEXT:  ret void
  return x;
}

// CHECK: define void @_Z5test1b(
// CHECK-EH: define void @_Z5test1b(
X test1(bool B) {
  // CHECK:      tail call void @_ZN1XC1Ev
  // CHECK-NEXT: ret void
  X x;
  if (B)
    return (x);
  return x;
  // CHECK-EH:      tail call void @_ZN1XC1Ev
  // CHECK-EH-NEXT: ret void
}

// CHECK: define void @_Z5test2b
// CHECK-EH: define void @_Z5test2b
X test2(bool B) {
  // No NRVO.

  X x;
  X y;
  if (B)
    return y;
  return x;

  // CHECK: call void @_ZN1XC1Ev
  // CHECK-NEXT: call void @_ZN1XC1Ev
  // CHECK: call void @_ZN1XC1ERKS_
  // CHECK: call void @_ZN1XC1ERKS_
  // CHECK: call void @_ZN1XD1Ev
  // CHECK: call void @_ZN1XD1Ev
  // CHECK: ret void

  // The block ordering in the -fexceptions IR is unfortunate.

  // CHECK-EH:      call void @_ZN1XC1Ev
  // CHECK-EH-NEXT: invoke void @_ZN1XC1Ev
  // -> %invoke.cont1, %lpad

  // %invoke.cont1:
  // CHECK-EH:      br i1
  // -> %if.then, %if.end

  // %if.then: returning 'x'
  // CHECK-EH:      invoke void @_ZN1XC1ERKS_
  // -> %cleanup, %lpad5

  // %invoke.cont: rethrow block for %eh.cleanup.
  // This really should be elsewhere in the function.
  // CHECK-EH:      call void @_Unwind_Resume_or_Rethrow
  // CHECK-EH-NEXT: unreachable

  // %lpad: landing pad for ctor of 'y', dtor of 'y'
  // CHECK-EH:      call i8* @llvm.eh.exception()
  // CHECK-EH: call i32 (i8*, i8*, ...)* @llvm.eh.selector
  // CHECK-EH-NEXT: br label
  // -> %eh.cleanup

  // %invoke.cont2: normal cleanup for 'x'
  // CHECK-EH:      call void @_ZN1XD1Ev
  // CHECK-EH-NEXT: ret void

  // %lpad5: landing pad for return copy ctors, EH cleanup for 'y'
  // CHECK-EH: invoke void @_ZN1XD1Ev
  // -> %eh.cleanup, %terminate.lpad

  // %if.end: returning 'y'
  // CHECK-EH: invoke void @_ZN1XC1ERKS_
  // -> %cleanup, %lpad5

  // %cleanup: normal cleanup for 'y'
  // CHECK-EH: invoke void @_ZN1XD1Ev
  // -> %invoke.cont2, %lpad

  // %eh.cleanup:  EH cleanup for 'x'
  // CHECK-EH: invoke void @_ZN1XD1Ev
  // -> %invoke.cont, %terminate.lpad

  // %terminate.lpad: terminate landing pad.
  // CHECK-EH:      call i8* @llvm.eh.exception()
  // CHECK-EH-NEXT: call i32 (i8*, i8*, ...)* @llvm.eh.selector
  // CHECK-EH-NEXT: call void @_ZSt9terminatev()
  // CHECK-EH-NEXT: unreachable

}

X test3(bool B) {
  // FIXME: We don't manage to apply NRVO here, although we could.
  {
    X y;
    return y;
  }
  X x;
  return x;
}

extern "C" void exit(int) throw();

// CHECK: define void @_Z5test4b
X test4(bool B) {
  {
    // CHECK: tail call void @_ZN1XC1Ev
    X x;
    // CHECK: br i1
    if (B)
      return x;
  }
  // CHECK: tail call void @_ZN1XD1Ev
  // CHECK: tail call void @exit(i32 1)
  exit(1);
}
