#!/bin/bash

# usage function
function usage() {
	echo -e "pecan.sh version 1.0"
	echo -e "Author: Qingkai Shi"
	echo -e "  If you want to report a bug or want to join me to contribute to it, please do not hasitate to contact qingkaishi@gmail.com."
	echo -e ""
	echo -e "Usage:"
	echo -e "  pecan.sh [i|a] [File]"
	echo ""
	echo -e "  -i      \t The input bit code file."
	echo -e "  -a      \t The input log file."
	echo -e "  -h      \t Print the help information."
}


while getopts "hi:a:" arg #":" means the previous option needs arguments
do
        case $arg in
             i)
                INPUT_FILE=$OPTARG #argmuments are stored in $OPTARG
		;;
             a)
                LOG_FILE=$OPTARG #argmuments are stored in $OPTARG
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

if [ -z "$INPUT_FILE" ] && [ -z "$LOG_FILE" ]; then
	usage
	exit -1
fi

if [ -f "$INPUT_FILE" ]; then
	opt -load dyckaa.so -basicaa -dyckaa -pecan-transformer $INPUT_FILE -o "$INPUT_FILE.t.bc"
	
	echo ""
	echo "The program has been transformed to $INPUT_FILE.t.bc."
	echo "Please add -ltrace when you compile it to be an executable file."
	echo "After executing your program, execute \"pecan.sh -a LOG_FILE_NAME\" to obtain predicted bugs."
	exit -1
elif [ -f "$LOG_FILE" ]; then
	pecan_log_analyzer $LOG_FILE "$LOG_FILE.results"
else
	echo "Error: $INPUT_FILE or $LOG_FILE do not exist!"
	exit -1
fi

