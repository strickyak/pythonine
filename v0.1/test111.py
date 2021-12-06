def ShowFStab():
    print 'Opening'
    fd = open('./../../../../../../../../../../etc/fstab', 'r')
    print 'Opened'
    for line in fd:
        print 'LINE'
        print line

ShowFStab()
print 'DONE.'
