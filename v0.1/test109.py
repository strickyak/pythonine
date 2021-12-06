def RunWhile():
    sum = 0
    i = 0
    while 1:
        print i
        if i==5:
            i = i + 1
            continue  # dont add 5.
        if i==12:
            break  # stop after 11.
        sum = sum + i
        print i
        print sum
        i = i + 1
    return sum


def RunFor():
    sum = 0
    for i in range(20):
        print i
        if i==5:
            continue  # dont add 5.
        if i==11:
            break  # stop after 10.
        sum = sum + i
        print i
        print sum
    return sum

assert RunWhile() == 61
assert RunFor() == 50
