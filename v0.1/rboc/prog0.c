#include "prog0.h"
#include "stringy1.h"
#include "alloc2.h"

#include "region.0.h"

char MemoryPool[1000];
int Next;

int PrintString(const char* s) {
  int n = strlen(s);
  printf("PrintString: %d# `%s'\n", n, s);
  return n;
}

int PrintString2(const char* a, const char* b) {
  printf("PrintString2: '%s' '%s'\n", a, b);
  return strlen(a) + strlen(b);
}

int main(int argc, const char* argv[]) {
  MemInit();
  Diagnostics();
  LoadRegion(1, "startup");
  Diagnostics();
  return 0;
  // Stop("main");

  PrintString("howdy");
  PrintString(StrCat("peanut", "butter"));
  return 0;
}
