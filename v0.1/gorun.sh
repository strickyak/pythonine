#!/bin/sh
set -eux

XFLAG=
case "$1" in
  -x ) XFLAG="-x" ; shift ;;
esac

case "$1" in
  *.go ) : ok ;;
  * ) echo "First arg not a .go file: $1" >&2 ; exit 13 ;;
esac

P="$1" ; shift
D=$(dirname "$P")
B=$(basename "$P" .go)

( cd "$D" && go $XFLAG build "$B.go" ) && "$D/$B" "$@"
