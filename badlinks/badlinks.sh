#!/bin/bash

if ! [ $# == 1 ]; then 
    echo "You need one argument"
    exit 1
fi
    
var1=$1
MYPWD=${PWD}
v=$MYPWD/$var1
 
if ! [ -d $v ]; then
    echo "You need to create directory"
    exit 1
fi    

find -L $v  -mtime +7 -type l -exec echo {} \;

exit $?
