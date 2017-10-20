#!/bin/bash
. ${PWD}/config.bash
i=0
while [[ $i -lt ${#dirs[@]} ]]; do
    dir=${PWD}/src/${dirs[i]}
	rm -f $dir/*.o
    i=$(($i+1))
done

rm -f ${PWD}/src/*.o
rm -rf inc/* lib/*.a bin/*.exec
