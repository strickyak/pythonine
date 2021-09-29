all: kmem emulate

kmem: kmem.c
	/opt/yak/cmoc/bin/cmoc --os9 kmem.c

emulate:
	os9 copy -r kmem /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/kmem
	os9 attr -per /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,CMDS/kmem
	:
	(echo 'kmem #32 </term') | os9 copy -l -r /dev/stdin  /home/strick/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2,STARTUP
	:
	(set -x ; cd ~/go/src/github.com/strickyak/doing_os9/gomar ; go run -x -tags=coco3,level2 gomar.go -boot drive/boot2coco3 -disk drive/disk2 -h0 drive/my-68SDC.VHD  --swi_fatal_coredump=1 2>_err)


clean:
	rm -f kmem
