def Run():
    a = [10, 20, 90, 40]
    it = a.__iter__()
    assert it.next() == 10
    assert it.next() == 20
    assert it.next() == 90
    assert it.next() == 40
    try:
        it.next()
    except as ex:
        print ex
        return ex
    return 9999
x = Run()
assert x == 'StopIter'
