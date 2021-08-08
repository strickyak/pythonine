def two(a, b):
    return a + b

def one(a):
    return a

def work():
    x = 0
    try:
        x = one(100, 200)
    except as ex:
        x = ex
    return x

z = work()
assert z=='bad_nargs'
print z
