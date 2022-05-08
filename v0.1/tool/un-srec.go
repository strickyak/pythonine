// +build main

/*
   un-srec.go reads Motorola S-Records on stdin,
   writes raw extracted binary to stdout.
*/
package main

import (
	"bufio"
	"fmt"
	"os"
)

func main() {
	sc := bufio.NewScanner(os.Stdin)
LOOP:
	for sc.Scan() {
		t := sc.Text()
		switch t[0] {
		case 'S':
			break
		default:
			panic(t)
		}
		switch t[1] {
		case '1':
			// skip 2 byte length & 4 byte addr
			s := t[8 : len(t)-2]
			b := make([]byte, len(s)/2)
			fmt.Sscanf(s, "%x", &b)
			_, err := os.Stdout.Write(b)
			if err != nil {
				panic(err)
			}
		case '9':
			break LOOP
		default:
			panic(t)
		}
	}
}
