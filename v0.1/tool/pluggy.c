#include <cmoc.h>
#include <assert.h>

#include "mmap.h"
#include "io.h"
#include "plugback.h"

int main() {
    word block1 = 0;
    byte err = AllocateBlock(1, &block1);
    assert(!err);
    assert(block1);

    word addr1 = 0;
    err = MapBlock(block1, 1, &addr1);
    assert(!err);
    assert(addr1);

    byte fd;
    err = FOpen("plug1.raw", false, &fd);
    assert(!err);

    word bytes_read;
    err = FRead(fd, (char*)addr1, 0x2000, &bytes_read);
    assert(!err);

    printf("bytes_read=%d\n", bytes_read);
    return 0;
}

word FuncOne() { // export FuncOne
    return 111;
}

word FuncTwo() { // export FuncTwo
    return 200;
}

void FuncPrint(word x) { // export FuncPrint
    printf("==> FuncPrint: %d\n", x);
}
