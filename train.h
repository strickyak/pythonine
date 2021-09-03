#ifndef PYTHONINE_TRAIN_H_
#define PYTHONINE_TRAIN_H_

#include "standard.h"
#include "octet.h"
#include "_generated_prim.h"

#define TRAIN_CONTINUED_CAP 64

#define EX_KEY_NOT_FOUND 71
#define EX_INDEX_OOB 72

struct TrainIterator {
  word p;
  byte i;
};

word NewTrain(byte cap, byte cls);
word NewTrainIter(word base, byte cls);

int TrainLen2(word train);
word TrainAddrOfNth(word train, int nth);
word TrainAddrOfAppend(word train);
word TrainGetNth(word train, int nth);
void TrainPutNth(word train, int nth, word value);
void TrainAppend(word train, word value);
void TrainIterStart(word train, struct TrainIterator *iter);
bool TrainIterMore(struct TrainIterator *iter);
word TrainIterNextAddr(struct TrainIterator *iter);
word TrainIterNext(struct TrainIterator *iter);
word TrainMapGet(word chain, word key);
word TrainMapGetOrDefault(word chain, word key, word dflt);
void TrainMapPut(word chain, word key, word value);
word TrainMapAddr(word chain, word key);
int TrainMapWhatNth(word chain, word key);

#endif
