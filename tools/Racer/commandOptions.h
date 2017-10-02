#ifndef LLVM_CLANG_COMMANDOPTIONS_H
#define LLVM_CLANG_COMMANDOPTIONS_H

#include "clang/Tooling/CommonOptionsParser.h"

static cl::OptionCategory RacerOptCat("Static Analysis Options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");

static cl::opt<bool> Symb("sym",cl::desc("Build and dump the Symbol Table"), cl::cat(RacerOptCat));
static cl::opt<bool> PA("pa",cl::desc("show pointer analysis info"), cl::cat(RacerOptCat));
static cl::opt<bool> HA("ha",cl::desc("show header analysis info"), cl::cat(RacerOptCat));
static cl::opt<bool> CG("cg",cl::desc("Call Graph Info"), cl::cat(RacerOptCat));
static cl::opt<std::string> FUNC1("m1",cl::desc("Initial method from which execution starts"), cl::value_desc("function name"),cl::cat(RacerOptCat));
static cl::opt<std::string> FUNC2("m2",cl::desc("Initial method from which execution starts"), cl::value_desc("function name"),cl::cat(RacerOptCat));

static cl::opt<bool> RA("ra",cl::desc("Data race analysis"), cl::cat(RacerOptCat));

enum DLevel {
  O, O1, O2, O3
};
cl::opt<DLevel> DebugLevel("dl", cl::desc("Choose Debug level:"),
  cl::values(
	     clEnumValN(O, "none", "No debugging"),
	     clEnumVal(O1, "Minimal debug info"),
	     clEnumVal(O2, "Expected debug info"),
	     clEnumVal(O3, "Extended debug info")), cl::cat(RacerOptCat));

#endif
