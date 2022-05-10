class Apple:
    def __init__(self, a):
        self.x = 100
        self.y = a
        self.z = 200

class Banana:
    def __init__(self, p, q):
        self.p = p
        self.q = q

try:
    a1 = Apple("an apple")
    a2 = Apple(a1)
    a3 = Apple(a2)
    b1 = Banana(404, a3)

    print(' * saving * ')
    savecluster(b1, "b1.clu")
    print(' * saved * ')

except Exception as ex:
    print('Exception')
    print(ex)
    print('Exception', ex)
print(' * ok *\n')
