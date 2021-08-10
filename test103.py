def identity_(a): return a
def add_(a, b): return a+b
def sum_(vec):
    z = 0
    for e in vec: z = add_(z, identity_(e))
    return z

def work():
    try:
        return sum_([10, 20, 30])
    except as ex:
        return 'BOGUS'
    return 'BOTTOM'

class Foo: def bar(self, x): return x+work()

assert Foo().bar(3) == 63
