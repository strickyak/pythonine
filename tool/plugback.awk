BEGIN {
    print "  SECTION code"
}
/^Symbol: _/ && NF==5 && $4=="=" {
    symbol[$2] = $5
}
/[{] [/][/] export/ {
    print "  EXPORT _" $NF
    print "_" $NF " EQU $" symbol["_" $NF]
}
END {
    # print "  EXPORT program_start"
    # print "program_start EQU $" symbol["_PlugInit"]

    print "  ENDSECTION code"
}
