def stuff():
    x = 100
    try:
        y = x + x
    except as e:
        assert 0 == 1

    assert y == 200

stuff()
