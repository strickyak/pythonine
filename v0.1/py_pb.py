KIND_TAG = 0
KIND_INT = 1
KIND_STR = 2
KIND_MESSAGE = 3


def ttag(num, kind):
    assert 1 <= num and num <= 29, num
    assert 0 <= kind and kind <= 3, kind
    return (num << 3) | (7 & kind)


def put_varint(v, x):
    while x > 127:
        v.append(128 | (127 & x))
        x = x >> 7
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
        x = 255 & ord(ch)
        v.append(x)


def put_start_message(v, ttag):
    assert (ttag & 7) == KIND_MESSAGE
    v.append(ttag)


def put_finish_message(v):
    v.append(0)


def bytevec2str(v):
    s = ''
    for x in v:
        assert 0 <= x and x <= 255, x
        s = s + chr(x)
    return s
