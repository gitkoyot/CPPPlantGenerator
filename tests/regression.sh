export LD_LIBRARY_PATH=/usr/lib/llvm-3.4/lib
CPPLANT=../CPPPlantGenerator
#set -x
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


