// train.c

#include "train.h"

#define SAY(S,X) (printf("%s: ", (S)), osay(X))
#define VERB(S,X) (printf("%s: ", (S)), osay(X))

word Equal(word, word);

word NewTrain(byte cap, byte cls) {
  word z = oalloc(cap, cls);
  assert(z);
  ocheckguards(z);
  Train_len2_Put(z, 0);
  return z;
}

int TrainLen2(word train) {
  int sum = 0;
  for (; train; train = Train_next(train)) {
    ocheckguards(train);
    sum += Train_len2(train);
  }
  return sum;
}
    
word TrainAddrOfNth(word train, int nth) {
  assert(train);
  assert(nth >= 0);
  for (; train; train = Train_next(train)) {
    int num_slots = (ocap(train) >> 1) - (Train_Size>>1);
    if (nth < num_slots) {
        return train+Train_Size+nth+nth;
    }
    nth -= num_slots;
  }
  return 0;
}

#define TRAIN_CONTINUED_CAP 64
word TrainAddrOfAppend(word train) {
  assert(train);
  word p;
  for (; train; train = Train_next(train)) {
    p = train;  // remember last wagon in p.
  }
  // Now p points to the last current wagon.
  byte cap = ocap(p);
  word num_slots = (cap>>1) - (Train_Size>>1);
  byte len2 = (byte) Train_len2(p);
  if (Train_len2(p) < num_slots) {
    // still more room in current wagon.
    Train_len2_Put(p, len2+1);
    return p + Train_Size + len2+len2;
  }
  // Must alloc another one.
  word z = oalloc(TRAIN_CONTINUED_CAP, ocls(p));
  Train_next_Put(p, z);
  Train_len2_Put(z, 1);
  return z+Train_Size;  // addr of flex portion.
}

word TrainGetNth(word train, int nth) {
  word addr = TrainAddrOfNth(train, nth);
  if (!addr) opanic(EX_INDEX_OOB);
  return ogetw(addr);
}

void TrainPutNth(word train, int nth, word value) {
  word addr = TrainAddrOfNth(train, nth);
  if (!addr) opanic(EX_INDEX_OOB);
  oputw(addr, value);
}

void TrainAppend(word train, word value) {
  word addr = TrainAddrOfAppend(train);
  oputw(addr, value);
}

void TrainIterStart(word train, struct TrainIterator *iter) {
  iter->p = train;
  iter->i = 0;
}

bool TrainIterMore(struct TrainIterator *iter) {
  if (!iter->p) return false;
  if (iter->i >= Train_len2(iter->p)) {
    iter->p = Train_next(iter->p);
  }
  return (iter->p != 0);
}
word TrainIterNextAddr(struct TrainIterator *iter) {
  byte j = iter->i;
  iter->i++;
  return iter->p + Train_Size + j+j;
}
word TrainIterNext(struct TrainIterator *iter) {
  return ogetw(TrainIterNextAddr(iter));
}

// Map

word TrainMapGet(word train, word key) {
  SAY("@@@@@ train", train);
  SAY("key", key);
  word addr = TrainMapAddr(train, key);
  if (!addr) opanic(EX_KEY_NOT_FOUND);
  SAY("FOUND; value", ogetw(addr));
  return ogetw(addr);
}
word TrainMapGetOrDefault(word train, word key, word dflt) {
  SAY("@@@@@ train", train);
  SAY("key", key);
  word addr = TrainMapAddr(train, key);
  if (!addr) return dflt;
  SAY("FOUND; value", ogetw(addr));
  return ogetw(addr);
}
void TrainMapPut(word train, word key, word value) {
  word addr = TrainMapAddr(train, key);
  if (addr) {
    oputw(addr, value);
  } else {
    addr = TrainAddrOfAppend(train);
    assert(addr);
    oputw(addr, key);
    addr = TrainAddrOfAppend(train);
    assert(addr);
  }
}
word TrainMapAddr(word train, word key) {
  SAY("@@@@@ train", train);
  SAY("key", key);

  { // First pass: ==
    struct TrainIterator it;
    TrainIterStart(train, &it);
    while (TrainIterMore(&it)) {
      word ikey = TrainIterNext(&it);
      assert(TrainIterMore(&it));
      word iaddr = TrainIterNextAddr(&it);
      SAY("iter_key", ikey);
      if (ikey == key) {
        VERB("FOUND at %04x\n", iaddr);
        return iaddr;
      }
    }
  }

  { // Second pass: Equal()
    struct TrainIterator it;
    TrainIterStart(train, &it);
    while (TrainIterMore(&it)) {
      word ikey = TrainIterNext(&it);
      assert(TrainIterMore(&it));
      word iaddr = TrainIterNextAddr(&it);
      SAY("iter_key", ikey);
      if (Equal(ikey, key)) {
        VERB("FOUND at %04x\n", iaddr);
        return iaddr;
      }
    }
  }

  SAY("NOT FOUND", key);
  return NIL;
}
