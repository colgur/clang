//== ArrayBoundChecker.cpp ------------------------------*- C++ -*--==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines ArrayBoundChecker, which is a path-sensitive check
// which looks for an out-of-bound array element access.
//
//===----------------------------------------------------------------------===//

#include "InternalChecks.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerVisitor.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ExprEngine.h"

using namespace clang;
using namespace ento;

namespace {
class ArrayBoundChecker : 
    public CheckerVisitor<ArrayBoundChecker> {      
  BuiltinBug *BT;
public:
  ArrayBoundChecker() : BT(0) {}
  static void *getTag() { static int x = 0; return &x; }
  void visitLocation(CheckerContext &C, const Stmt *S, SVal l, bool isLoad);
};
}

void ento::RegisterArrayBoundChecker(ExprEngine &Eng) {
  Eng.registerCheck(new ArrayBoundChecker());
}

void ArrayBoundChecker::visitLocation(CheckerContext &C, const Stmt *S, SVal l,
                                      bool isLoad) {
  // Check for out of bound array element access.
  const MemRegion *R = l.getAsRegion();
  if (!R)
    return;

  const ElementRegion *ER = dyn_cast<ElementRegion>(R);
  if (!ER)
    return;

  // Get the index of the accessed element.
  DefinedOrUnknownSVal Idx = cast<DefinedOrUnknownSVal>(ER->getIndex());

  // Zero index is always in bound, this also passes ElementRegions created for
  // pointer casts.
  if (Idx.isZeroConstant())
    return;

  const GRState *state = C.getState();

  // Get the size of the array.
  DefinedOrUnknownSVal NumElements 
    = C.getStoreManager().getSizeInElements(state, ER->getSuperRegion(), 
                                            ER->getValueType());

  const GRState *StInBound = state->assumeInBound(Idx, NumElements, true);
  const GRState *StOutBound = state->assumeInBound(Idx, NumElements, false);
  if (StOutBound && !StInBound) {
    ExplodedNode *N = C.generateSink(StOutBound);
    if (!N)
      return;
  
    if (!BT)
      BT = new BuiltinBug("Out-of-bound array access",
                       "Access out-of-bound array element (buffer overflow)");

    // FIXME: It would be nice to eventually make this diagnostic more clear,
    // e.g., by referencing the original declaration or by saying *why* this
    // reference is outside the range.

    // Generate a report for this bug.
    RangedBugReport *report = 
      new RangedBugReport(*BT, BT->getDescription(), N);

    report->addRange(S->getSourceRange());
    C.EmitReport(report);
    return;
  }
  
  // Array bound check succeeded.  From this point forward the array bound
  // should always succeed.
  assert(StInBound);
  C.addTransition(StInBound);
}
