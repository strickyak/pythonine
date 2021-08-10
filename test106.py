def Run():
    i = 0
    x = {}
    while i <= 100:
        x[i] = (i, i)
        i = i + 1
    try:
        it = x.__iter__()
        total = 0
        while 1:
            total = total + it.next()
    except as ex:
        assert ex == 'StopIter'
        return total


z = Run()
assert z == 5050
