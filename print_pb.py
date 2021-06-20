import sys


class MessagePrinter:
    def __init__(self, v):
        self.v = v
        self.i = 0
        self.pre = '  '

    def VarInt(self):
        shift = 0
        z = 0
        while True:
            a = self.v[self.i]
            self.i += 1
            z += ((a & 127) >> shift)
            shift += 7
            if not (a & 128):
                return z

    def Mess(self):
        while True:
            ttag = self.v[self.i]
            self.i += 1
            if not ttag:
                return

            tag, kind = ttag >> 3, ttag & 7
            assert 1 <= tag <= 29
            assert 1 <= kind <= 3

            if kind == 1:
                x = self.VarInt()
                print '%s %d: int %d' % (self.pre, tag, x)
            elif kind == 2:
                n = self.VarInt()
                x = self.v[self.i:self.i + n]
                self.i += n
                s = ''.join([chr(e) for e in x])
                print '%s %d: str %s' % (self.pre, tag, repr(s))
            elif kind == 3:
                print '%s %d: message:' % (self.pre, tag)
                old = self.pre
                self.pre += '  '
                self.Mess()
                self.pre = old
            else:
                raise 0


vec = [ord(ch) for ch in sys.stdin.read()]
MessagePrinter(vec).Mess()
