#include "clang/Driver/Driver.h"
#include "clang/Frontend/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/System/Host.h"
#include "llvm/System/Path.h"

using namespace clang;
using namespace clang::driver;

llvm::sys::Path GetExecutablePath(const char *Argv0) {
  // This just needs to be some symbol in the binary; C++ doesn't
  // allow taking the address of ::main however.
  void *MainAddr = (void*) (intptr_t) GetExecutablePath;
  return llvm::sys::Path::GetMainExecutable(Argv0, MainAddr);
}

int main(int argc, const char **argv, char * const *envp) {
  //  void *MainAddr = (void*) (intptr_t) GetExecutablePath;
  llvm::sys::Path Path = GetExecutablePath(argv[0]);
  TextDiagnosticPrinter *DiagClient =
    new TextDiagnosticPrinter(llvm::errs(), DiagnosticOptions());

  llvm::IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  Diagnostic Diags(DiagID, DiagClient);
  Driver TheDriver(Path.str(), llvm::sys::getHostTriple(),
                   "a.out", /*IsProduction=*/false, /*CXXIsProduction=*/false,
                   Diags);
  TheDriver.setTitle("Clang-based Toy Analyzer");
  TheDriver.PrintHelp(true);

  return( 0 );
}
