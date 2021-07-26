class Apple(object):
    #def __init__(self, abc):
    #    self.x = abc
    #    return self
    def __init__(self):
        pass

    def SetX(self, x):
        self.unused = 0
        self.x = x

    def SetY(self, y):
        self.y = y

    def Sum(self):
        return self.x + self.y


a = Apple()
a.__init__()
a.SetX(20)
a.SetY(3)
assert 23 == a.Sum()
print a.Sum()
