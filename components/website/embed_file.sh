#!/bin/bash
set -e
# $1 is name of c value
# $2 is input file
# $3 is output file
# echo "#include \"website.h\"" > $2
xxd -n _ff_$1 -i $2 > $3
echo "char *get_${1}(){return (char*)_ff_${1};}" >> $3
echo "unsigned long get_${1}_size(){return (unsigned long)_ff_${1}_len;}" >> $3