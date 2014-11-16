#!/bin/bash

for file in *.ll
do
    mkdir .test
    outputfile=.test/${file%.*}.bc
    llvm-as $file -o $outputfile
    
    option=`head -1 $file`
    option=${option:1}

    canary $option $outputfile
done

rm -rf .test/
