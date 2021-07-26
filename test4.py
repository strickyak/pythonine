a = 3


def munge(x):
    z = x + x
    return z + a


def sumsq(x, y):
    return x * x + y * y


assert munge(50) == 103
assert sumsq(3, 4) == 25
print sumsq(3, 4)
