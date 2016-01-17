#!/bin/bash

if [ "$1" == "--help" ]; then
    echo "Usage: $0 [release [<qt-version> [buildonly] ] ]"
    echo "       $0 --help"
    exit 0
fi

debug="CONFIG+=debug"
relname="debug"
if [ "$1" == "release" ]; then
    debug=""
    relname="release"
fi

spec="linux-g++"
if command -v clang &>/dev/null
then
    spec="linux-clang"
fi
if [ "$CC" == "clang" ] || [ "$CXX" == "clang++" ]
then
    spec="linux-clang"
fi
if [ "$CC" == "gcc" ] || [ "$CXX" == "g++" ]
then
    spec="linux-g++"
fi

qtver=""
if [ "$2" != "" ]
then
    qtver="-qt=$2"
fi

cwd="$PWD"
cd "`dirname "$0"`"

echo -e "\e[1m==> Building intranet with $spec ...\e[0m"
mkdir -p build-$relname/
cd build-$relname/
qmake $qtver -spec "$spec" ../KFG-Intranet.pro $debug || exit 1
make -j4 || exit 1

export "PATH=$PWD:$PATH"
export "LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH"
for path in "$PWD"/*/
do
    export "PATH=$path:$PATH"
    export "LD_LIBRARY_PATH=$path:$LD_LIBRARY_PATH"
done
echo $PATH
echo $LD_LIBRARY_PATH
cd "$cwd"

if [ "$3" != "buildonly" ]
then
    echo -e "\e[1m==> Starting intranet ...\e[0m"
    echo -e "run \"`dirname "$0"`/config.ini\"\nbt\nq\ny" | gdb -q "`dirname "$0"`/build-$relname/Intranet/intranet"
fi
