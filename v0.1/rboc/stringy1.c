#include "prog0.h"
#include "stringy1.h"
#include "alloc2.h"

#include "region.1.h"

char* StrDup(const char* s) {
  int n = strlen(s);
  char* z = Malloc(n+1);
  strcpy(z, s);
  return z;
}

char* StrCat(const char*a, const char*b) {
  char buf[100];
  PrintString2("StrCat first string:", a);
  PrintString2("StrCat second string:", a);
  strcpy(buf, a);
  strcat(buf, b);
  char* z = StrDup(buf);
  PrintString2("StrCat creates string:", z);
  return z;
}
