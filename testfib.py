def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)


# 1, 1, 2, 3, 5, 8.
fib6 = fib(6)
print fib6
assert fib6 == 8
