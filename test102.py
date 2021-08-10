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
