# test Parens
a = 5
assert a==5
a = (11)
assert a==11
a = (((99)))
assert a==99
a = (12,)
assert len(a)==1
assert a[0] == 12
a = (((92,)))
assert len(a)==1
assert a[0] == 92
a = ()
assert len(a)==0
a = (25,26)
assert len(a)==2
assert a[0] == 25
assert a[1] == 26
a = (35,36,)
assert len(a)==2
assert a[0] == 35
assert a[1] == 36
a = ((52,),)
assert len(a)==1
assert len(a[0])==1
assert a[0][0] == 52
