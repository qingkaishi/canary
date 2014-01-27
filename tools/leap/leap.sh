#!/bin/bash

# usage function
function usage() {
	echo -e "leap.sh version 1.0"
	echo -e "Author: Qingkai Shi"
	echo -e "  If you want to report a bug or want to join me to contribute to it, please do not hasitate to contact qingkaishi@gmail.com."
	echo -e ""
	echo -e "Usage:"
	echo -e "  leap.sh -i <bitcode file>"
	echo ""
	echo -e "  -i      \t The input bit code file."
	echo -e "  -h      \t Print the help information."
}


while getopts "hi:" arg #":" means the previous option needs arguments
do
        case $arg in
             i)
                INPUT_FILE=$OPTARG #argmuments are stored in $OPTARG
		;;
	     h)
		usage
		exit -1
		;;
             ?)  #unknown args
		usage
		exit -1
                ;;
        esac
done

if [ -z "$INPUT_FILE" ]; then
	usage
	exit -1
fi

if [ -f "$INPUT_FILE" ]; then
	opt -load dyckaa.so -basicaa -dyckaa -leap-transformer $INPUT_FILE -o "$INPUT_FILE.t.bc"
	
	echo ""
	echo "The program has been transformed to $INPUT_FILE.t.bc."
	echo "If you want a record version, please add -lleaprecord when you compile it to be an executable file."
	echo "If you want a replay version, please add -lreplay when you compile it to be an executable file."
else
	echo "Error: $INPUT_FILE does not exist!"
	exit -1
fi

