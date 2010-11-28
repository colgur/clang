##===- projects/gaius/Makefile -----------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

CLANG_LEVEL := ../..

TOOLNAME = gaius
NO_INSTALL = 1

# No plugins, optimize startup time.
TOOL_NO_EXPORTS = 1

LINK_COMPONENTS := support
USEDLIBS = clangFrontend.a clangDriver.a \
	   clangParse.a clangLex.a clangBasic.a

include $(CLANG_LEVEL)/Makefile
