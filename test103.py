def one(a):
    return a

def two(a, b):
    return a+b

def work():
    x = 0
    try:
        x = one(100, 200)
    except as ex:
        x = ex
    return x

z = work()
print z
