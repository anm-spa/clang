// RUN: mkdir -p %T/xtudir2
// RUN: %clang_cc1 -triple x86_64-pc-linux-gnu -emit-pch -o %T/xtudir2/xtu-other.c.ast %S/Inputs/xtu-other.c
// RUN: cp %S/Inputs/externalFnMap2_usr.txt %T/xtudir2/externalFnMap.txt
// RUN: %clang_cc1 -triple x86_64-pc-linux-gnu -fsyntax-only -analyze -analyzer-checker=core,debug.ExprInspection -analyzer-config xtu-dir=%T/xtudir2 -analyzer-config use-usr=true -analyzer-config reanalyze-xtu-visited=true -verify %s

void clang_analyzer_eval(int);

typedef struct {
  int a;
  int b;
} foobar;

static int s1 = 21;

int f(int);
int enumcheck(void);
int static_check(void);

enum A { x,
         y,
         z };

extern foobar fb;

int getkey();

int main() {
  clang_analyzer_eval(f(5) == 1);             // expected-warning{{TRUE}}
  clang_analyzer_eval(x == 0);                // expected-warning{{TRUE}}
  clang_analyzer_eval(enumcheck() == 42);     // expected-warning{{TRUE}}

  return getkey();
}

//TEST
//reporting error
//in a macro
struct S;
int g(struct S *);
void test_macro(void) {
  g(0); // expected-warning@Inputs/xtu-other.c:31 {{Access to field 'a' results in a dereference of a null pointer (loaded from variable 'ctx')}}
}
