#!/bin/bash

set -eux

set $( for x in test*.py; do echo $x | sed -e 's/test//' -e 's/[.]py//' ; done )

rm -rf /tmp/pythonine-tests
mkdir /tmp/pythonine-tests

for t
do
  t2=$(basename $t | tr _ -)
  make clean
  make T=$t _build
  cp -v test$t.py /tmp/pythonine-tests/test$t2.py
  cp -v test$t.bc /tmp/pythonine-tests/test$t2.bc
  # cp -v test$t.dump /tmp/pythonine-tests/test$t2.dump
done

# HINT
#    start with NOS9_6809_L2_v030300_coco3_80d_thin_284k_free.dsk
#    copy to make disk2-for-py, and delete everything under /NITROS9
# cd /tmp/pythonine.tests ;  for x in *.bc ; do os9 copy $x ~/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2-for-py,PY/$x ; done
#
# os9 copy runpy ~/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2-for-py,CMDS/runpy
# os9 attr -per ~/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2-for-py,CMDS/runpy


