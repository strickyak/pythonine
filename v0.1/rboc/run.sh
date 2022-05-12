#!/bin/bash
set -eux

for PHASE
do
case $PHASE in

1)
for n in 0 1 2 3 ; do echo // >region.$n.h ; done
cc -Werror -Wall *.c && ./a.out && ci-l *.[ch]
;;

2)
for n in 0 1 2 3 ; do echo // >region.$n.h ; done
cmoc -Werror -i --os9 prog0.c stringy1.c alloc2.c mmap.c ur.c readbuf.c
os9 ident prog0
;;

3)
../gomar/launch.sh prog0 /dev/null
;;

4)
python rboc.py 1:stringy1.h 2:alloc2.h
;;

5)
cmoc -D"PASS_ONE=1" -Werror -i --os9 prog0.c mmap.c ur.c readbuf.c
os9 ident prog0
;;

6)
  cmoc -c --org=C000 -D"PASS_ONE=1" -Werror -i --coco stringy1.c

;;
esac
done
