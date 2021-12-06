class Num:
    def __init__(self, a):
        self.x = a

    def Print(self):
        print self.x

    def Next(self, increment):
        if self.x >= 16000:
            return Num(0)
        else:
            return Num(self.x + increment)

obj = Num(0)
while 1:
    obj.Print()
    Num(obj.x).Print()
    obj = obj.Next(1)
