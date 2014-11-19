#!/bin/bash

num=0
for file in *.ll
do
    num=$((num+1))

    mkdir .test
    clear
    outputfile=.test/${file%.*}.bc
    llvm-as $file -o $outputfile
    
    option=`head -1 $file`
    option=${option:1}

    echo "Test: canary $option $outputfile"
    echo "==============================================="
    canary $option $outputfile -o $outputfile
    exitcode=$?
    if [ $exitcode != 0 ]; then
        echo "==============================================="
        echo "Test Fail! Exit code: $exitcode."
        exit -1;
    fi
done

rm -rf .test/

echo "==============================================="
echo "Congradulations! All ($num) tests passed!"
