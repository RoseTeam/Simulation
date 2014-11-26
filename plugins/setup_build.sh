#!/bin/bash


echo
echo "Automatically create Eclipse project file from CMakeLists.txt"
echo "see:"
echo "http://sgpsproject.sourceforge.net/JavierVGomez/index.php/How_to_configure_a_C/C%2B%2B_project_with_Eclipse_and_CMake"
echo
# working directory
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

set -o xtrace #print each command before executing it
if ! [ -a $DIR/build]; then
 mkdir $DIR/build
fi

cd $DIR/build
cmake -G"Eclipse CDT4 - Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug ../src
set +o xtrace #stop printing each command before executing it	
