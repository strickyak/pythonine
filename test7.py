class Apple(object):
    def JustAMethod(self, abc):
        self.unused = abc
        self.x = abc
        self.y = abc
        return self


def SetX(a, x):
    a.unused = 0
    a.x = x


def SetY(a, y):
    a.y = y


def Sum(a):
    return a.x + a.y


a = Apple()
SetX(a, 20)
SetY(a, 3)
assert 23 == Sum(a)
print Sum(a)
