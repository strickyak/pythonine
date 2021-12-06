# pythonine -- a tiny python for 6809 with Nitr/OS9 Level2

This is a tiny python for a 64k memory space.  Initially I'm targeting
Motorola 6809 under OS9 Level2 or NitrOS9 Level2.

_Etymology:_ Programmers use the word "pythonic", while in biology the
adjective is "pythonine".  And the pun is that it ends in "09" like 6809
and Basic09.

Currently compilation of .py files is only on Linux.  It compiles to
a bytecode file, which you must then copy to your Nitr/OS9 machine,
and run with the bytecode interpreter `runpy`.  Only a small subset of
python2 is supported.

## v0.1: Premade

Premade binary `runpy` and bytecode file `bc` are in the `v0.1/premade`
directory.  Section "Basic Demo" below tells you to copy `runpy` to your
`CMDS` directory, and `bc` to your current OS9 working directory.

As an optimization (in both time and space), the binary assumes that the
Direct Page is $00.  This is always the case on OS9 Level 2, and never
the case on OS9 Level 1.  So this can only run on Level 2.  It's going
to need almost the full 64k memory space, anyway.

If you want to see what is inside the bytecode file, you can browse
`premade/bc.listing`.  It uses my own tiny protocol buffer format
(message defs in `bc.proto`) and my own bytecodes (defined in `prim.txt`).

## v0.1: make test

Try `make test`, but it won't work for you.  You'll have to fix paths
or mimic my environment, and that won't be easy.  It might be better to
wait for v0.2.

## v0.1: Basic Demo

V0.1 is roughly what I demoed at VCF-SE 2021 (Vintage Computer Festival
-- Southeast, Atlanta, September 2021).  Compile `v0.1/test_basic.py`
into bytecodes `bc` and the runtime into the OS9 binary module `runpy`.
Then copy these onto an OS9 Level 2 (v3.3.0) disk, with `runpy` in the
`CMDS` directory, and `bc` in the Current Working Directory.  Do not
allow any character translations; both files are binary.  After booting
into a shell, run `runpy #128`.  It is hard-wired to load `bc` (which
takes a long time).

How I build them:

```
make T=_basic _build _runpy  # (then hit ^C after it runs a while)
make T=_basic emu            # (optional, to try it in my emulator)
```

They run (very slowly) a small version of Basic that I wrote in the
currently-supported subset of Python.  It preloads this Basic program,
to check the Collatz conjecture:

```
10 REM Prove the Collatz Conjecture
30 LET a = 0                # Loop for natural nums with A
50 LET a = a + 1
80 PRINT
100 LET b = a               # start testing a chain with A
110 PRINT b
120 IF b < 2 THEN 50        # next a
130 IF b % 2 = 0 THEN 500
200 LET b = 3 * b + 1       # if odd
220 GOTO 110
500 LET b = b / 2           # if even
520 GOTO 110
```

At the prompt, you can type `LIST` or `RUN` or `SHELL` or `BYE`.  Or you
can define/redefine a line by starting with a line number.  Or you
can type a `PRINT` statement.  But the Basic only supports the five
statement types used above, and only in the trivial way demonstrated.
There is not (yet) any way to interrupt a running program and get back
to the Basic prompt.

## Next Version

v0.2 is in progress.

## Relationship to MicroPython: None.

This is an original project, from scratch, and bears no relationship to
MicroPython or CircuitPython.  As I understand them, they are a significant
subset of Python3, and require around 250kB of memory.

This project is a tiny subset of Python2, designed to run in less than
64kB of RAM.

## License: MIT
