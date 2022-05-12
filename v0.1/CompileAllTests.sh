#!/bin/bash

set -eux

set $( for x in test*.py; do echo $x | sed -e 's/test//' -e 's/[.]py//' ; done )

rm -rf /tmp/pythonine-tests
mkdir /tmp/pythonine-tests

for t
do
  make clean
  make T=$t _build && cp -v test$t.py test$t.bc test$t.dump /tmp/pythonine-tests/
done

# HINT
#    start with NOS9_6809_L2_v030300_coco3_80d_thin_284k_free.dsk
#    copy to make disk2-for-py, and delete everything under /NITROS9
# cd /tmp/pythonine.tests ;  for x in *.bc ; do os9 copy $x ~/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2-for-py,PY/$x ; done
#
# os9 copy runpy ~/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2-for-py,CMDS/runpy
# os9 attr -per ~/go/src/github.com/strickyak/doing_os9/gomar/drive/disk2-for-py,CMDS/runpy


