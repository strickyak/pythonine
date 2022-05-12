class Apple:
    def __init__(self, a):
        self.x = 100
        self.y = a
        self.z = 200

class Banana:
    def __init__(self, p, q):
        self.p = p
        self.q = q

a1 = Apple("an apple")
a2 = Apple(a1)
a3 = Apple(a2)
b1 = Banana(404, a3)

print(' * saving * ')
savecluster(b1, "b1.clu")
print(' * saved * ')

x = loadcluster("b1.clu")
print(' * loaded * ')
print(x)
print(' * printed * ')

assert x.p == 404
assert x.q.x == 100
assert x.q.y.z == 200
assert x.q.y.y.y == "an apple"

print(' * ok *\n')
