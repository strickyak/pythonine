#ifndef PYTHONINE_CHAIN_H_
#define PYTHONINE_CHAIN_H_

#include "runtime.h"

#define CHAIN_CHUNK_SIZE 64
#define CHAIN_CHUNK_WORDS (CHAIN_CHUNK_SIZE / 2)
#define CHAIN_CHUNK_SLOTS ((CHAIN_CHUNK_WORDS)-1)

#define INF 0xFF /* infinity, not a valid byte index */
#define NIL ((word)0)

// Create a new empty list.
// The raw chain is used directly as a list.
// It can have 0 to 254 (inclusive) items.
word NewList();
// Create a new empty dict.
// It is a chain with alternating keys and values as entries.
// It can have 0 to 127 (inclusive) items.
// There is no hashing; it uses linear search.
word NewDict();

// Return the address of the Nth item in the chain (which must exist).
word ChainAddrOfNth(word chain, byte nth);
// Add a new entry to the chain, and return its address.
// Its value will be initialized to None.
word ChainAddrOfAppend(word chain);

// Helper structure used to iterate a chain.
struct ChainIterator {
  word obj;
  byte offset;
  byte count;
  byte length;
};

// Functions for iterating.
void ChainIterStart(word chain, struct ChainIterator *iter);
word ChainIterNextAddr(word chain, struct ChainIterator *iter);
word ChainIterNext(word chain, struct ChainIterator *iter);
bool ChainIterMore(word chain, struct ChainIterator *iter);

// Functions for using the chain as a list.
// The (python) len() of the dict is (Dict_len2(chain)).
word ChainGetNth(word chain, byte nth);
void ChainPutNth(word chain, byte nth, word value);
void ChainAppend(word chain, word value);

// Functions for using the chain as a dict.
// The (python) len() of the dict is (Dict_len2(chain)/2).
byte ChainDictWhatNth(word chain, word key);
word ChainDictAddr(word chain, word key);
word ChainDictGet(word chain, word key);
void ChainDictPut(word chain, word key, word value);

#endif
