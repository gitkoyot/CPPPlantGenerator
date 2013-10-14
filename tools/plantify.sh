#!/bin/bash

TOOL=/home/koyot/eclipse-workspace/CPPPlantGenerator/CPPPlantGenerator
CXXOPTIONS="-pthread -x c++ -I/usr/include/i386-linux-gnu/c++/4.8 -std=c++11"

function show_help {
 echo "$0 [-h|-?] [-v] [-f filename] [-d directory] "
}


# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
input_file=""
input_dir=""
verbose=0

while getopts "h?vf:d:" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    v)  verbose=1
        ;;
    f)  input_file=$OPTARG
        ;;
	  d) input_dir=$OPTARG
		  ;;
  esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

echo "verbose=$verbose, input_file='$input_file', Leftovers: $@"

if [ $verbose -gt 0 ]; then
	echo "Verbose on."
	export CPPPLANTDIAG=1
	export CPPLANTINNERFILE=1
	export CPPLANTDEPENDENCIES=1
fi

if [ -e "$input_file" ]; then
	$TOOL $input_file $CXXOPTIONS > ${input_file%.*}"_plant.h"
fi

if [ -d "$input_dir" ]; then
	cd $input_dir
	for source_file in `ls -1 *.cpp` 
	do
		if [ 1 -eq $verbose ]; then
			echo "Processing $source_file "
		fi
		echo "/*" > ${source_file%.*}"_plant.h"
		$TOOL $source_file  $CXXOPTIONS >> ${source_file%.*}"_plant.h"
		echo "*/" >> ${source_file%.*}"_plant.h"
	done
   cd -
fi

