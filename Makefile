T=2

# all except emu
all: _build _octet_test _chain_test _runpy

test: 1 2 3 4 5 fib 6 7 8
	echo

1:
	make T=1 _run emu
2:
	make T=2 _run emu
3:
	make T=3 _run emu
4:
	make T=4 _run emu
5:
	make T=5 _run emu
6:
	make T=6 _run emu
7:
	make T=7 _run emu
8:
	make T=8 _run emu
fib:
	make T=fib _run emu



_build:
	python2 compile_proto.py < bc.proto -c > _generated_proto.h
	python2 compile_proto.py < bc.proto -p > _generated_proto.py
	python2 generate_core.py < core.txt > _generated_core.h
	python2 compile_pyth09.py < test$T.py > test$T.bc
	cp test$T.bc bc
	python2 print_pb.py bc.proto _generated_core.h < bc | tee test$T.dump | tee ,dump
	cc -g -o runpy.bin runpy.c readbuf.c arith.c runtime.c chain.c defs.c pb2.c octet.c

_runpy:
	./runpy.bin

_chain_test:
	python2 generate_core.py < core.txt > _generated_core.h
	cc -DDONT_SAY -g -o chain_test.bin chain_test.c defs.c chain.c octet.c
	./chain_test.bin

_octet_test:
	cc -g -o octet_test.bin octet_test.c octet.c
	./octet_test.bin

# to debug
db: _build
	echo 'run test$T.bc' > /tmp/test$T.gdb
	gdb -x /tmp/test$T.gdb ./runpy.bin

format:
	clang-format --style=Google -i *.h *.c
	yapf -i *.py

clean:
	rm -f a.out *.bin *.pyc *.bc _* *.s *.o.list *.map *.link ,*

ci:
	mkdir -p RCS
	ci -l -m/dev/null -t/dev/null -q *.py *.h *.c *.proto *.txt Makefile

#######################################
FLOPPY=drive/boot2coco3
HARD=drive/disk2
SDC=drive/my-68SDC.VHD

emu: __always__
	rm -f runpy
	go run ~/go/src/github.com/strickyak/doing_os9/gomar/cmocly/cmocly.go -cmoc /opt/yak/cmoc/bin/cmoc  -o runpy runpy.c readbuf.c runtime.c chain.c pb2.c arith.c defs.c octet.c
	:
	os9 copy -r test$T.bc /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,bc
	:
	os9 copy -r runpy /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	:
	echo "runpy #128" | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	set -eux; (sleep 300 ; killall gomar 2>/dev/null) & (cd ~/go/src/github.com/strickyak/doing_os9/gomar ; go run -x -tags=coco3,level2 gomar.go -boot ${FLOPPY} -disk ${HARD} -h0 ${SDC}  2>_ | tee /dev/stderr | grep FINISHED) || echo "*** CRASHED ($$?) ***" >&2


mooh:
	cp -f ~/go/src/github.com/strickyak/doing_os9/gomar/drive/mooh-sdcard-co42.img /tmp/_img
	dd bs=512 skip=512 count=7650 if=/tmp/_img of=/tmp/_drive
	os9 copy -r test$T.bc /tmp/_drive,bc
	os9 copy -r runpy /tmp/_drive,CMDS/runpy
	os9 attr -per /tmp/_drive,CMDS/runpy
	:
	os9 copy -r ~/go/src/github.com/strickyak/doing_os9/picol/ncl.bin /tmp/_drive,CMDS/ncl
	os9 attr -per /tmp/_drive,CMDS/ncl
	:
	dd bs=512 seek=512 count=7650 if=/tmp/_drive of=/tmp/_img
	sync
	test -b /dev/sdb && sudo dd bs=1024k if=/tmp/_img of=/dev/sdb
	sync


__always__:
