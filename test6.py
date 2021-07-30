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

class Banana(object):
    def Greet(self):
        return "Hello!"

a = Apple()
assert 7 == a.Sum()
assert 3 == a.x
assert 4 == a.y
a.SetX(20)
a.SetY(3)
assert 23 == a.Sum()
print a.Sum()
print Banana().Greet()
