all: kmem emulate

pluggy: pluggy.c
	/opt/yak/cmoc/bin/cmoc --os9 --function-stack=0 -S pluggy.c
	lwasm --obj --6809 --list=pluggy.list -o pluggy.o pluggy.s
	/opt/yak/cmoc/bin/cmoc --os9 --function-stack=0 -S io.c
	lwasm --obj --6809 --list=io.list -o io.o io.s
	/opt/yak/cmoc/bin/cmoc --os9 --function-stack=0 -S mmap.c
	lwasm --obj --6809 --list=mmap.list -o mmap.o mmap.s
	/opt/yak/cmoc/bin/cmoc --os9 -i -o pluggy pluggy.o io.o mmap.o
	:
	grep '// export' pluggy.c | sed 's|{ *// *export|; // export|' > plugback.h
	:
# lwlink --6809 --list=pluggy.list -o pluggy pluggy.o
# : : : : go run ~/go/src/github.com/strickyak/doing_os9/gomar/cmocly/cmocly.go -cmoc /opt/yak/cmoc/bin/cmoc pluggy.c

plug1.srec : plug1.c pluggy
	awk -f plugback.awk pluggy.map pluggy.c > plugback.s
	: cmoc --os9 -S plug1.c
	cmoc -S plug1.s
	lwasm --obj --6809 --list=plug1.list -o plug1.o plug1.s
	(echo " SECTION code"; echo " EXPORT program_start"; echo "program_start EQU 0") > pluginit.s
	cmoc --srec -i plug1.o plugback.s pluginit.s
	go run un-srec.go < plug1.srec > plug1.raw

kmem: kmem.c mmap.h mmap.c
	/opt/yak/cmoc/bin/cmoc --os9 kmem.c mmap.c

emulate:
	os9 copy -r kmem /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/kmem
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/kmem
	:
	(echo 'kmem #32 </term') | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	:
	(set -x ; cd ~/go/src/github.com/strickyak/doing_os9/gomar ; go run -x -tags=coco3,level2 gomar.go -boot drive/boot2coco3 -disk drive/disk2 -h0 drive/my-68SDC.VHD  --swi_fatal_coredump=1 2>_err)


clean:
	rm -f kmem
