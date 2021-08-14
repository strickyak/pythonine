T=1
C=42

# all except emu
all: _build _runpy
# all: _build _octet_test _chain_test _runpy

test: 1 2 3 4 5 fib 6 7 8 101 102 103 104 105 106 107 108 109 110
	echo

1:
	make T=1 _build _runpy emu
2:
	make T=2 _build _runpy emu
3:
	make T=3 _build _runpy emu
4:
	make T=4 _build _runpy emu
5:
	make T=5 _build _runpy emu
6:
	make T=6 _build _runpy emu
7:
	make T=7 _build _runpy emu
8:
	make T=8 _build _runpy emu
fib:
	make T=fib _build _runpy emu
101:
	make T=101 _build _runpy emu
102:
	make T=102 _build _runpy emu
103:
	make T=103 _build _runpy emu
104:
	make T=104 _build _runpy emu
105:
	make T=105 _build _runpy emu
106:
	make T=106 _build _runpy emu
107:
	make T=107 _build _runpy emu
108:
	make T=108 _build _runpy emu
109:
	make T=109 _build _runpy emu
110:
	make T=110 _build _runpy emu


_build:
	python2 compile_proto.py < bc.proto -c > _generated_proto.h
	python2 compile_proto.py < bc.proto -p > _generated_proto.py
	python2 generate_prim.py < prim.txt > _generated_prim.h
	python2 compile_pyth09.py < test$T.py > test$T.bc
	cp test$T.bc bc
	python2 print_pb.py bc.proto _generated_prim.h < bc | tee test$T.dump | tee ,dump
	cc -g -o runpy.bin runpy.c readbuf.c arith.c runtime.c data.c chain.c osetjmp.c defs.c pb2.c octet.c

_build_ev:
	python2 compile_proto.py < bc.proto -c > _generated_proto.h
	python2 compile_proto.py < bc.proto -p > _generated_proto.py
	python2 compile_proto.py < bc.proto -k > _generated_proto.const
	python2 generate_prim.py < prim.txt > _generated_prim.h
	rm -f _ev.py
	sed -n '/((( eval9 (((/,/))) eval9 )))/p' py_pb.py compile_pyth09.py | sed 's/\<P\>[.]//g' | python2 replace_constants.py *.const > _ev.py
	chmod -w _ev.py
	python2 _ev.py < test$T.py > test$T.bc
	cp test$T.bc bc
	python2 print_pb.py bc.proto _generated_prim.h < bc | tee test$T.dump | tee ,dump
	cc -g -o runpy.bin runpy.c readbuf.c arith.c runtime.c data.c chain.c osetjmp.c defs.c pb2.c octet.c

_runpy:
	./runpy.bin

_chain_test:
	python2 generate_prim.py < prim.txt > _generated_prim.h
	cc -DDONT_SAY -g -o chain_test.bin chain_test.c osetjmp.c defs.c data.c chain.c octet.c
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
	go run ~/go/src/github.com/strickyak/doing_os9/gomar/cmocly/cmocly.go -cmoc /opt/yak/cmoc/bin/cmoc  -o runpy runpy.c readbuf.c runtime.c data.c chain.c pb2.c arith.c osetjmp.c defs.c octet.c
	:
	os9 copy -r test$T.bc /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,bc
	:
	os9 copy -r runpy /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	:
	(echo 'runpy #128'; echo 'dir /d1') | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	: T=$T
	set -eux; (sleep 300 ; killall gomar 2>/dev/null) & B=$$! ; (cd ~/go/src/github.com/strickyak/doing_os9/gomar ; go run -x -tags=coco3,level2 gomar.go -boot ${FLOPPY} -disk ${HARD} -h0 ${SDC}  2>_ | tee /dev/stderr | grep FINISHED) && kill $$B || echo "*** CRASHED ($$?) ***" >&2
	ls -l runpy


temu: __always__
	rm -f runpy
	go run ~/go/src/github.com/strickyak/doing_os9/gomar/cmocly/cmocly.go -cmoc /opt/yak/cmoc/bin/cmoc  -o runpy runpy.c readbuf.c runtime.c data.c chain.c pb2.c arith.c osetjmp.c defs.c octet.c
	:
	os9 copy -r test$T.bc /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,bc
	:
	os9 copy -r runpy /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	:
	(echo 'runpy #128'; echo 'dir /d1') | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	: T=$T
	set -eux; (sleep 300 ; killall gomar 2>/dev/null) & B=$$! ; (cd ~/go/src/github.com/strickyak/doing_os9/gomar ; go run -x -tags=coco3,level2,trace gomar.go -t=12000000 -vv=a --trigger_os9='(?i:fork.*runpy)' -borges=/home/strick/go/src/github.com/strickyak/doing_os9/borges/ -boot ${FLOPPY} -disk ${HARD} -h0 ${SDC}  2>_ | tee /dev/stderr | grep FINISHED) && kill $$B || echo "*** CRASHED ($$?) ***" >&2
	ls -l runpy


mooh:
	cp -f ~/go/src/github.com/strickyak/doing_os9/gomar/drive/mooh-sdcard-co$C.img /tmp/_img
	dd bs=512 skip=512 count=7650 if=/tmp/_img of=/tmp/_drive
	os9 copy -r test$T.bc /tmp/_drive,bc
	os9 copy -r runpy /tmp/_drive,CMDS/runpy
	os9 attr -per /tmp/_drive,CMDS/runpy
	:
	(echo 'echo nando'; echo 'tmode .1 pau=0'; echo 'runpy #128') | os9 copy -l -r /dev/stdin  /tmp/_drive,STARTUP
	:
	os9 copy -r ~/go/src/github.com/strickyak/doing_os9/picol/ncl.bin /tmp/_drive,CMDS/ncl
	os9 attr -per /tmp/_drive,CMDS/ncl
	:
	dd bs=512 seek=512 count=7650 if=/tmp/_drive of=/tmp/_img
	sync
	test -b /dev/sdb && sudo dd bs=1024k if=/tmp/_img of=/dev/sdb
	sync; sync; sync
	ls -l runpy


__always__:
