def stuff(x):
    if x==99:
        raise 99
    try:
        if x==7:
            return 13
        if x<1:
            raise 42
        y = x + x
        return y
    except as e:
        return e

    assert y == 200

assert 200 == stuff(100)
assert 13 == stuff(7)
assert 42 == stuff(0)

def try_99():
    try:
        stuff(99)
    except:
        return 1
    return 0

assert try_99()
