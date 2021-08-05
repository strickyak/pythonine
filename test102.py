def nando():
    try:
        try:
            raise 3
        except ex:
            raise ex+30
    except ex:
        return ex+300
    return 0

assert nando() == 333
