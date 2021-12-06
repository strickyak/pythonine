#include "types.h"
#include "plugback.h"

void AddThem() {
    word one = FuncOne();
    word two = FuncTwo();
    FuncPrint(one + two);
}

static word Dispatch[1];

word PlugInit() { // init PlugInit
    Dispatch[0] = AddThem;
    return (word)&Dispatch;
}
