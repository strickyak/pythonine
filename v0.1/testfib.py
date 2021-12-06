def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)


# 1, 1, 2, 3, 5, 8, 13.
fib7 = fib(7)
print fib7
assert fib7 == 13
