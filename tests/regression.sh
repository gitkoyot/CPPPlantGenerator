#!/bin/bash
export LD_LIBRARY_PATH=/usr/lib/llvm-3.4/lib
CPPLANT=../CPPPlantGenerator
#set -x

function show_help {
 echo "$0 [-h|-?] [-v] "
}


# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
verbose=0

while getopts "h?v" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    v)  verbose=1
        ;;
  esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

if [ $verbose -gt 0 ]; then
	echo "Verbose on."
	export CPPPLANTDIAG=1
	export CPPLANTINNERFILE=1
	export CPPLANTDEPENDENCIES=1
fi


for testName in `ls -1v test*.cpp ` 
do 
filename_wop=$(basename "$testName")
extension="${filename_wop##*.}"
filename="${filename_wop%.*}"
 
echo -n "Starting test $filename ..."

$CPPLANT $filename_wop -pthread -x c++ -I/usr/include/i386-linux-gnu/c++/4.8 -std=c++11 > ${filename}_actual.txt

result=`diff ${filename}_expected.txt ${filename}_actual.txt | wc -l`

if [ ! -e ${filename}_actual.txt -o $result -gt  0 ]; 
then
printf "\033[31m FAILED \n\033[0m"
else
printf "\033[32m OK \n\033[0m"
fi

done


