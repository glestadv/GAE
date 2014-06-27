#!/bin/sh
# needs c++filt and addr2line, part of binutils

# $0 [-s] crash.txt executable-with-debuginfo
# example: ../prettify-crash.sh ~/.glestadv/gae-crash.txt source/game/glestadv

if [ "$1" = "-s" -a $# = 3 ]; then
	# simple mode
	crash=$2
	prog=$3
elif [ $# = 2 ]; then
	crash=$1
	prog=$2
else
	crash="$HOME/.glestadv/gae-crash.txt"
	prog="source/game/glestadv"
fi
if [ ! -f $crash -o ! -f $prog ]; then
	echo "can't find crashlog or program"
	exit 0
fi

# simple mode
if [ "$1" = "-s" ]; then
	grep -o -P '\[0x[0-9a-f]+\]' $crash | tr -d '[]' | xargs addr2line -e $prog -fpC
	exit 0
fi

tmp=`mktemp`
cp $crash $tmp

funcs=`grep -o -P '\(.*\+0x[0-9a-f]+\)' $crash | tr -d '()' | cut -d'+' -f1`
for f in $funcs; do
	sed -i "s/$f/$(c++filt $f)/" $tmp
done

addrs=`grep -o -P '\[0x[0-9a-f]+\]' $crash | tr -d '[]'`
for addr in $addrs; do
	sed -i "s|$addr|$(addr2line -e $prog $addr)|" $tmp
done

cat $tmp
rm $tmp
