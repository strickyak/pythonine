T=1
C=42
CFILES= runpy.c readbuf.c runtime.c data.c train.c pb2.c arith.c osetjmp.c defs.c octet.c rawio.c saver.c

# all except emu
all: _build _runpy
# all: _build _octet_test _runpy

test: 1 2 3 4 5 fib 6 7 8 101 102 103 104 105 106 107 108 109 110 111 114
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
111:
	make T=111 _build _runpy emu
114:
	make T=114 _build _runpy emu


_build:
	python3 compile_proto.py < bc.proto -c > _generated_proto.h
	python3 compile_proto.py < bc.proto -p > _generated_proto.py
	python3 generate_prim.py < prim.txt > _generated_prim.h
	cpp -DBIG -DUNIX compile_pyth09.py | python3 replace_constants.py *.const | grep -v '^#' > _big_unix.py
	python3 _big_unix.py < test$T.py > test$T.bc
	cp test$T.bc bc
	#broken# python3 print_pb.py bc.proto _generated_prim.h < bc | tee test$T.dump | tee ,dump
	cc -g -o runpy.bin $(CFILES)

small_unix:
	python3 compile_proto.py < bc.proto -c > _generated_proto.h
	python3 compile_proto.py < bc.proto -p > _generated_proto.py
	python3 compile_proto.py < bc.proto -k > _generated_proto.const
	python3 generate_prim.py < prim.txt > _generated_prim.h
	rm -f _small_unix.py
	# cpp -DSMALL -DUNIX compile_pyth09.py | sed 's/\<P\>[.]//g' | python3 replace_constants.py *.const > _big.py
	cpp -DSMALL -DUNIX compile_pyth09.py | grep -v '^#' | python3 replace_constants.py *.const > _small_unix.py
	chmod -w _small_unix.py
	python3 _small_unix.py < test1.py > test1.bc
	cp test1.bc bc
	#broken# python3 print_pb.py bc.proto _generated_prim.h < bc | tee test1.dump | tee ,dump
	cc -g -o runpy.bin $(CFILES)
	./runpy.bin

small_coco:
	python3 compile_proto.py < bc.proto -c > _generated_proto.h
	python3 compile_proto.py < bc.proto -p > _generated_proto.py
	python3 compile_proto.py < bc.proto -k > _generated_proto.const
	python3 generate_prim.py < prim.txt > _generated_prim.h
	rm -f _small_coco.py
	# cpp -DSMALL -DNORMAL compile_pyth09.py | sed 's/\<P\>[.]//g' | python3 replace_constants.py *.const > _small_coco.py
	cpp -DSMALL -DCOCO compile_pyth09.py | grep -v '^#' | python3 replace_constants.py *.const > _small_coco.py
	chmod -w _small_coco.py
	#
	#
	#
	python3 _big_unix.py < _small_coco.py > _small_coco.bc
	cp _small_coco.bc bc
	#broken# python3 print_pb.py bc.proto _generated_prim.h < bc | tee _small_coco.dump | tee ,dump
	cc -g -o runpy.bin $(CFILES)
	:
	./runpy.bin < test1.py > test1.by_small_coco.bc

_runpy:
	./runpy.bin bc

_test_train:
	python3 generate_prim.py < prim.txt > _generated_prim.h
	cc -I. -DDONT_SAY -g -o train_test.bin testc/test_train.c osetjmp.c defs.c data.c train.c octet.c runtime.c readbuf.c pb2.c
	./train_test.bin

_test_dict:
	python3 generate_prim.py < prim.txt > _generated_prim.h
	cc -I. -DDONT_SAY -g -o dict_test.bin testc/test_dict.c osetjmp.c defs.c data.c train.c octet.c runtime.c readbuf.c pb2.c
	./dict_test.bin

_test_int:
	python3 generate_prim.py < prim.txt > _generated_prim.h
	cc -I. -DDONT_SAY -g -o int_test.bin testc/test_int.c osetjmp.c defs.c data.c train.c octet.c runtime.c readbuf.c pb2.c
	./int_test.bin

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
	rm -rf __pycache__
	rm -f bc runpy a.out *.bin *.pyc *.bc *.s *.s-orig *.o *.o.list *.map *.link *.dump _* ,*

ci:
	mkdir -p RCS
	ci -l -m/dev/null -t/dev/null -q *.py *.h *.c *.proto *.txt Makefile

#######################################
FLOPPY=drive/boot2coco3
HARD=drive/disk2
SDC=drive/my-68SDC.VHD
SDCARD=sdb

launch: __always__
	sh ./gorun.sh ~/go/src/github.com/strickyak/doing_os9/gomar/cmocly/cmocly.go -cmoc /opt/yak/cmoc/bin/cmoc  -o runpy $(CFILES)
	:
	cp -fv "test$T.bc" "bc"
	:
	: # TTL=300s sh -x ../../doing_os9/gomar/launch.sh runpy /dev/null bc
	TTL=900s sh -x ../../doing_os9/gomar/launch.sh "--map $$PWD/runpy.map" runpy list-and-run.txt "test$T.bc"

emu: __always__
	rm -f runpy
	sh ./gorun.sh ~/go/src/github.com/strickyak/doing_os9/gomar/cmocly/cmocly.go -cmoc /opt/yak/cmoc/bin/cmoc  -o runpy $(CFILES)
	:
	os9 copy -r test$T.bc /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,bc
	:
	os9 copy -r runpy /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	:
	: : : -os9 makdir /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,etc
	: : : os9 copy -l -r /etc/fstab /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,etc/fstab
	(echo 'runpy #128 </term'; echo 'dir /d1') | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	: T=$T
	set -eux; (sleep 300 ; killall gomar 2>/dev/null) & B=$$! ; (cd ~/go/src/github.com/strickyak/doing_os9/gomar ; sh ./gorun.sh -x -tags=coco3,level2 gomar.go -boot ${FLOPPY} -disk ${HARD} -h0 ${SDC}  --swi_fatal_coredump=1 2>_ | tee /dev/stderr | grep FINISHED) && kill $$B || echo "*** CRASHED ($$?) ***" >&2
	ls -l runpy


kmem: __always__
	cd tool && make kmem
	os9 copy -r tool/kmem /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/kmem
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/kmem
	(echo 'echo Nando Kmem') | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	(set -x ; cd ~/go/src/github.com/strickyak/doing_os9/gomar ; sh ./gorun.sh -x -tags=coco3,level2 gomar.go -boot ${FLOPPY} -disk ${HARD} -h0 ${SDC}  --swi_fatal_coredump=1 2>/dev/null)


lemu: __always__
	rm -f runpy
	sh ./gorun.sh ~/go/src/github.com/strickyak/doing_os9/gomar/cmocly/cmocly.go -cmoc /opt/yak/cmoc/bin/cmoc  -o runpy $(CFILES)
	:
	os9 copy -r test$T.bc /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,bc
	:
	os9 copy -r runpy /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	:
	: : : -os9 makdir /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,etc
	: : : os9 copy -l -r /etc/fstab /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,etc/fstab
	(echo 'runpy #128 </term'; echo 'dir /d1') | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	: T=$T
	(set -x ; cd ~/go/src/github.com/strickyak/doing_os9/gomar ; sh ./gorun.sh -x -tags=coco3,level2 gomar.go -boot ${FLOPPY} -disk ${HARD} -h0 ${SDC}  --swi_fatal_coredump=1 2>/dev/null)
	ls -l runpy


temu: __always__
	rm -f runpy
	sh ./gorun.sh ~/go/src/github.com/strickyak/doing_os9/gomar/cmocly/cmocly.go -cmoc /opt/yak/cmoc/bin/cmoc  -o runpy $(CFILES)
	:
	os9 copy -r test$T.bc /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,bc
	:
	os9 copy -r runpy /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/runpy
	:
	: : : -os9 makdir /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,etc
	: : : os9 copy -l -r /etc/fstab /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,etc/fstab
	(echo 'runpy #128 </term'; echo 'dir /d1') | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	: T=$T
	set -eux; (sleep 300 ; killall gomar 2>/dev/null) & B=$$! ; (cd ~/go/src/github.com/strickyak/doing_os9/gomar ; sh ./gorun.sh -x -tags=coco3,level2,trace gomar.go -t=12000000 -vv=a --trigger_os9='(?i:fork.*runpy)' -borges=/home/strick/go/src/github.com/strickyak/doing_os9/borges/ -boot ${FLOPPY} -disk ${HARD} -h0 ${SDC}  2>_ | tee /dev/stderr | grep FINISHED) && kill $$B || echo "*** CRASHED ($$?) ***" >&2
	ls -l runpy


mooh:
	cp -f ~/go/src/github.com/strickyak/doing_os9/gomar/drive/mooh-sdcard-co$C.img /tmp/_img
	dd bs=512 skip=512 count=7650 if=/tmp/_img of=/tmp/_drive
	os9 copy -r test$T.bc /tmp/_drive,bc
	os9 copy -r runpy /tmp/_drive,CMDS/runpy
	os9 attr -per /tmp/_drive,CMDS/runpy
	:
	(echo 'echo nando'; echo 'tmode .1 pau=0'; echo 'runpy #128 </term') | os9 copy -l -r /dev/stdin  /tmp/_drive,STARTUP
	:
	os9 copy -r ~/go/src/github.com/strickyak/doing_os9/picol/ncl.bin /tmp/_drive,CMDS/ncl
	os9 attr -per /tmp/_drive,CMDS/ncl
	:
	dd bs=512 seek=512 count=7650 if=/tmp/_drive of=/tmp/_img
	sync
	test -b /dev/${SDCARD} && sudo dd bs=1024k if=/tmp/_img of=/dev/${SDCARD}
	sync; sync; sync
	ls -l runpy

sdc:
	cp -f ~/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2.orig /tmp/disk2
	for junk in basic09 runb cobbler asm disasm os9gen format picol p9 xyz mpi megaread ; do os9 del "/tmp/disk2,CMDS/$$junk" ; done
	os9 copy -r runpy /tmp/disk2,cmds/runpy
	os9 attr -per /tmp/disk2,cmds/runpy
	os9 copy -r bc /tmp/disk2,bc
	sudo cp -f /tmp/disk2 /mnt/PY.DSK

__always__:
