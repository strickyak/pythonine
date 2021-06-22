T=1

all: _build _octet_test _chain_test run

_build:
	python2 compile_proto.py < bc.proto -c > _generated_proto.h
	python2 compile_proto.py < bc.proto -p > _generated_proto.py
	python2 build_core.py < core.txt > _generated_core.h
	python compile_pyth09.py < test$T.py > test$T.bc
	cc -g -o runpy.bin runpy.c runtime.c chain.c pb.c octet.c

run:
	./runpy.bin test$T.bc

_chain_test:
	python2 build_core.py < core.txt > _generated_core.h
	cc -g -o chain_test.bin chain_test.c runtime.c chain.c pb.c octet.c
	./chain_test.bin

_octet_test:
	cc -g -o octet_test.bin octet_test.c octet.c
	./octet_test.bin

db: _build
	echo 'run test$T.bc' > /tmp/test$T.gdb
	gdb -x /tmp/test$T.gdb ./runpy.bin

format:
	clang-format --style=Google -i *.h *.c
	yapf -i *.py

clean:
	rm -f a.out *.bin *.pyc *.bc _*

ci:
	mkdir -p RCS
	ci -l -m/dev/null -t/dev/null -q *.py *.h *.c *.proto *.txt Makefile
