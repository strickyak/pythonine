def nando():
    try:
        try:
            raise 3
        except as ex:
            raise ex+30
    except as ex:
        return ex+300
    return 0

assert nando() == 333

def frodo():
    try:
        raise (4, 6), 100
    except as (aa, bb), cc:
        return aa * bb + cc

assert frodo() == 124
