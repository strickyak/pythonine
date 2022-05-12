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
