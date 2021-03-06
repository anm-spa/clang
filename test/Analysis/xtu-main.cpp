// RUN: mkdir -p %T/xtudir
// RUN: %clang_cc1 -triple x86_64-pc-linux-gnu -emit-pch -o %T/xtudir/xtu-other.cpp.ast %S/Inputs/xtu-other.cpp
// RUN: %clang_cc1 -triple x86_64-pc-linux-gnu -emit-pch -o %T/xtudir/xtu-chain.cpp.ast %S/Inputs/xtu-chain.cpp
// RUN: cp %S/Inputs/externalFnMap.txt %T/xtudir/
// RUN: %clang_cc1 -triple x86_64-pc-linux-gnu -fsyntax-only -analyze -analyzer-checker=core,debug.ExprInspection -analyzer-config xtu-dir=%T/xtudir -analyzer-config reanalyze-xtu-visited=true -verify %s

void clang_analyzer_eval(int);

int f(int);
int g(int);
int h(int);

int callback_to_main(int x) { return x + 1; }

namespace myns {
int fns(int x);

namespace embed_ns {
int fens(int x);
}

class embed_cls {
public:
  int fecl(int x);
};
}

class mycls {
public:
  int fcl(int x);
  static int fscl(int x);

  class embed_cls2 {
  public:
    int fecl2(int x);
  };
};

namespace chns {
int chf1(int x);
}
//test for a crash
//when inlining typedefs
typedef struct AVBuffer avt;
int avtSize(void);

int main() {
  clang_analyzer_eval(f(3) == 2); // expected-warning{{TRUE}}
  clang_analyzer_eval(f(4) == 3); // expected-warning{{TRUE}}
  clang_analyzer_eval(f(5) == 3); // expected-warning{{FALSE}}
  clang_analyzer_eval(g(4) == 6); // expected-warning{{TRUE}}
  clang_analyzer_eval(h(2) == 8); // expected-warning{{TRUE}}
  clang_analyzer_eval(myns::fns(2) == 9);                   // expected-warning{{TRUE}}
  clang_analyzer_eval(myns::embed_ns::fens(2) == -1);       // expected-warning{{TRUE}}
  clang_analyzer_eval(mycls().fcl(1) == 6);                 // expected-warning{{TRUE}}
  clang_analyzer_eval(mycls::fscl(1) == 7);                 // expected-warning{{TRUE}}
  clang_analyzer_eval(myns::embed_cls().fecl(1) == -6);     // expected-warning{{TRUE}}
  clang_analyzer_eval(mycls::embed_cls2().fecl2(0) == -11); // expected-warning{{TRUE}}
  clang_analyzer_eval(chns::chf1(4) == 12); // expected-warning{{TRUE}}
  clang_analyzer_eval(avtSize() == 4); // expected-warning{{TRUE}}
}
