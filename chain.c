#include <assert.h>
#include <stdio.h>

#include "chain.h"

static word init_chain_guts(word chain) {
  word guts = oalloc(CHAIN_CHUNK_SIZE, C_Array);
  Chain_root_Put(chain, guts);
  return chain;
}
word NewList() { return init_chain_guts(List_NEW()); }
word NewDict() { return init_chain_guts(Dict_NEW()); }

// returns value at Nth
word ChainAddrOfNth(word chain, byte nth) {
  assert(nth < INF);
  byte len2 = Chain_len2(chain);
  assert(nth < len2);

  word p = Chain_root(chain);
  while (1) {
    assert(p);
    assert(ocap(p) == CHAIN_CHUNK_SIZE);
    assert(ocls(p) == C_Array);

    if (nth < CHAIN_CHUNK_SLOTS)
      return p + 2 * nth;  // return address of desired slot.

    p = ogetw(p + CHAIN_CHUNK_SIZE - 2);
    nth -= CHAIN_CHUNK_SLOTS;
  }
}

word ChainAddrOfAppend(word chain) {
  byte n = Chain_len2(chain);
  assert(n < 254);  // max is 254, after +1.
  Chain_len2_Put(chain, n + 1);

  word linkaddr = Chain_root_LEA(chain);
  word p = ogetw(linkaddr);
  while (1) {
    if (!p) {
      p = oalloc(CHAIN_CHUNK_SIZE, C_Array);
      oputw(linkaddr, p);
    }
    assert(ocap(p) == CHAIN_CHUNK_SIZE);
    assert(ocls(p) == C_Array);

    if (n < CHAIN_CHUNK_SLOTS)
      return p + 2 * n;  // return address of desired slot.

    linkaddr = p + CHAIN_CHUNK_SIZE - 2;
    p = ogetw(linkaddr);
    n -= CHAIN_CHUNK_SLOTS;
  }
}

void ChainIterStart(word chain, struct ChainIterator *iter) {
  iter->obj = Chain_root(chain);
  iter->offset = 0;  // address the first one.
  iter->count = 0;
  iter->length = Chain_len2(chain);
}
word ChainIterNextAddr(word chain, struct ChainIterator *iter) {
  // Move to the next chunk if we need to.
  // This does not advance iter->count.
  if (iter->offset >= CHAIN_CHUNK_SLOTS) {
    word linkaddr = iter->obj + CHAIN_CHUNK_SIZE - 2;
    word p = GetW(linkaddr);
    iter->obj = p;
    iter->offset = 0;
  }

  word z = iter->obj + 2 * (word)iter->offset;
  ++iter->offset;  // address the next one.
  ++iter->count;
  return z;
}
word ChainIterNext(word chain, struct ChainIterator *iter) {
  word addr = ChainIterNextAddr(chain, iter);
  assert(addr);
  return GetW(addr);
}
bool ChainIterMore(word chain, struct ChainIterator *iter) {
  return iter->count < iter->length;
}

void ChainIterEach(word chain, void (*fn)(word)) {
  struct ChainIterator it;
  ChainIterStart(chain, &it);
  while (ChainIterMore(chain, &it)) {
    fn(ChainIterNext(chain, &it));
  }
}
void ChainIterDump(word chain, void (*fn)(word)) {
  fprintf(stderr, "ChainIterDump:\n");
  ChainIterEach(chain, osay);
  fprintf(stderr, "\n");
}

word ChainGetNth(word chain, byte nth) {
  word addr = ChainAddrOfNth(chain, nth);
  assert(addr);
  return GetW(addr);
}
void ChainPutNth(word chain, byte nth, word value) {
  word addr = ChainAddrOfNth(chain, nth);
  assert(addr);
  PutW(addr, value);
}
void ChainAppend(word chain, word value) {
  word addr = ChainAddrOfAppend(chain);
  assert(addr);
  PutW(addr, value);
}

byte ChainDictWhatNth(word chain, word key) {
  struct ChainIterator it;
  ChainIterStart(chain, &it);
  byte i = 0;
  while (ChainIterMore(chain, &it)) {
    assert(i + 1 < INF);
    word ikey = ChainIterNext(chain, &it);
    assert(ChainIterMore(chain, &it));
    (void)ChainIterNext(chain, &it);
    if (ikey == key) return i;
    i += 2;
  }
  return INF;
}

word ChainDictAddr(word chain, word key) {
  struct ChainIterator it;
  ChainIterStart(chain, &it);
  while (ChainIterMore(chain, &it)) {
    word ikey = ChainIterNext(chain, &it);
    assert(ChainIterMore(chain, &it));
    word iaddr = ChainIterNextAddr(chain, &it);
    if (ikey == key) return iaddr;
  }
  return NIL;
}
word ChainDictGet(word chain, word key) {
  word addr = ChainDictAddr(chain, key);
  if (!addr) return NIL;
  return GetW(addr);
}
void ChainDictPut(word chain, word key, word value) {
  word addr = ChainDictAddr(chain, key);
  if (!addr) {
    addr = ChainAddrOfAppend(chain);
    assert(addr);
    PutW(addr, key);
    addr = ChainAddrOfAppend(chain);
    assert(addr);
  }
  PutW(addr, value);
}
