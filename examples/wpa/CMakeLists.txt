set(LLVM_USED_LIBS
  clangIndex
  clangFrontend
  clangDriver
  clangSema
  clangAnalysis
  clangSerialization
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  clangRewrite
  clangAST
  clangParse
  clangLex
  clangBasic)

set( LLVM_LINK_COMPONENTS
  bitreader
  mc
  core
  )

add_clang_executable(clang-wpa
  clang-wpa.cpp
  )
add_dependencies(clang-wpa clang-headers)
