def TestOr2():
    print 11
    assert 0 == (0 or 0)
    assert 1 == (0 or 1)
    assert 1 == (1 or 0)
    assert 1 == (1 or 1)

def TestOrNot2():
    print 12
    assert 1 == (not 0 or not 0)
    assert 1 == (not 0 or not 1)
    assert 1 == (not 1 or not 0)
    assert 0 == (not 1 or not 1)

def TestOr3():
    print 22
    assert 0 == (0 or 0 or 0)
    assert 1 == (0 or 1 or 0)
    assert 1 == (1 or 0 or 0)
    assert 1 == (1 or 1 or 0)
    assert 1 == (0 or 0 or 1)
    assert 1 == (0 or 1 or 1)
    assert 1 == (1 or 0 or 1)
    assert 1 == (1 or 1 or 1)

def TestAnd2():
    print 33
    assert 0 == (0 and 0)
    assert 0 == (0 and 1)
    assert 0 == (1 and 0)
    assert 1 == (1 and 1)

def TestAnd3():
    print 44
    assert 0 == (0 and 0 and 0)
    assert 0 == (0 and 1 and 0)
    assert 0 == (1 and 0 and 0)
    assert 0 == (1 and 1 and 0)
    assert 0 == (0 and 0 and 1)
    assert 0 == (0 and 1 and 1)
    assert 0 == (1 and 0 and 1)
    assert 1 == (1 and 1 and 1)

def NeverCalled():
    assert 111 == 222

# assert short-circuting.
print 55
assert 1 == (1 or NeverCalled())
assert 0 == (0 and NeverCalled())

TestOr2()
TestOrNot2()
TestOr3()
TestAnd2()
TestAnd3()

assert 1 == 1 or 0 and 0 or 1
assert 1 == not 0 or 0 and 0 or not 0
