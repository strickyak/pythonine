KIND_TAG = 0
KIND_INT = 1
KIND_STR = 2
KIND_MESSAGE = 3


def ttag(num, kind):
    assert 1 <= num <= 29, num
    assert 0 <= kind <= 3, kind
    return (num << 3) | (7 & kind)


def put_varint(v, x):
    while x > 0x7F:
        v.append(0x80 | 0x7F & x)
        x >>= 7
    v.append(x)


def put_int(v, ttag, x):
    assert (ttag & 7) == KIND_INT
    assert 0 <= x, x
    v.append(ttag)
    put_varint(v, x)


def put_str(v, ttag, s):
    assert (ttag & 7) == KIND_STR
    v.append(ttag)
    put_varint(v, len(s))
    for ch in s:
        x = 0xFF & ord(ch)
        v.append(x)


def put_start_message(v, ttag):
    assert (ttag & 7) == KIND_MESSAGE
    v.append(ttag)


def put_finish_message(v):
    v.append(0)


def bytevec2str(v):
    s = ''
    for x in v:
        assert 0 <= x <= 255, x
        s += chr(x)
    return s
