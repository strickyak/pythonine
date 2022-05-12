#ifndef _UR_H_
#define _UR_H_

#define MAPPED_BLOCK_ADDR 0xC000

#if unix
#include <stdio.h>
#include <string.h>
#else
#include <cmoc.h>
//#include <assert.h>
#endif

typedef unsigned char byte;
typedef unsigned int word;

word CALLER(byte region, word address, byte num_args, ...);

void LoadRegion(byte region, const char* filename);
void Stop(const char* why);
void Diagnostics();

#define assert(C) { if (!(C)) { \
  printf("[%s:%d: FAIL] ", __FILE__, __LINE__); Stop("assert"); }}

#define asserteq(X,Y) { word _x_ = (word)(X); word _y_ = (word)(Y); \
  printf("[%s:%d: %x==%x] ", __FILE__, __LINE__, _x_, _y_); \
  if (_x_ != _y_ ) Stop("asserteq"); }

#define checkerr(X) { byte _e_ = (X); \
  printf(" checkerr"); \
  asserteq(_e_, 0); }

#endif // _UR_H_
