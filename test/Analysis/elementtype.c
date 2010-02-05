// RUN: %clang_cc1 -analyze -analyzer-experimental-internal-checks -analyzer-check-objc-mem -analyzer-store=region %s

typedef struct added_obj_st {
  int type;
} ADDED_OBJ;

// Test if we are using the canonical type for ElementRegion.
void f() {
  ADDED_OBJ *ao[4]={((void*)0),((void*)0),((void*)0),((void*)0)};
  if (ao[0] != ((void*)0))   {
    ao[0]->type=0;
  }
}
