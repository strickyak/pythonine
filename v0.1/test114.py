def run():
    for i in range(100):
        for j in range(10):
            try:
                try:
                    try:
                        break
                    except Exception as ex:
                        pass
                except Exception as ex:
                    pass
            except Exception as ex:
                pass
        print i

print 'start'
run()
print 'finish'
