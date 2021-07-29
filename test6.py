class Apple(object):
    def __init__(self):
        self.x = 3
        self.y = 4

    def SetX(self, x):
        self.unused = 0
        self.x = x

    def SetY(self, y):
        self.y = y

    def Sum(self):
        return self.x + self.y


a = Apple()
assert 7 == a.Sum()
a.SetX(20)
a.SetY(3)
assert 23 == a.Sum()
print a.Sum()
